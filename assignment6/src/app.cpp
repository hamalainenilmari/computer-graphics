
#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include "tiny_obj_loader.h"

#include <filesystem>
#include <iostream>

#include "im3d.h"
#include "im3d_math.h"
#include "implot.h"
#include "lodepng.h"

#include "app.h"

#include "fmt/core.h"
#include "Utils.h"

#ifndef CS3100_TTF_PATH
#define CS3100_TTF_PATH "roboto_mono.ttf"
#endif

//------------------------------------------------------------------------
// Static data members

App* App::static_instance_ = 0; // The single instance we allow of App

GLFWkeyfun          App::default_key_callback_ = nullptr;
GLFWmousebuttonfun  App::default_mouse_button_callback_ = nullptr;
GLFWcursorposfun    App::default_cursor_pos_callback_ = nullptr;
GLFWdropfun         App::default_drop_callback_ = nullptr;
GLFWscrollfun       App::default_scroll_callback_ = nullptr;

//------------------------------------------------------------------------

// defined in im3d_opengl33.cpp
bool Im3d_Init();
void Im3d_NewFrame(GLFWwindow* window,
    int width,
    int height,
    const Matrix4f& world_to_view,
    const Matrix4f& view_to_clip,
    float dt,
    float mousex,
    float mousey);
void Im3d_EndFrame();
namespace Im3d
{
    inline void Vertex(const Vector3f& v) { Im3d::Vertex(v(0), v(1), v(2)); }
    inline void Vertex(const Vector4f& v) { Im3d::Vertex(v(0), v(1), v(2), v(3)); }
    const char* GetGlEnumString(GLenum _enum);
}
#define glAssert(call) \
        do { \
            (call); \
            GLenum err = glGetError(); \
            if (err != GL_NO_ERROR) { \
                fail(fmt::format("glAssert failed: {}, {}, {}, {}", #call, __FILE__, __LINE__, Im3d::GetGlEnumString(err))); \
            } \
        } while (0)

//------------------------------------------------------------------------

static Matrix3f getOrientation(const Vector3f& forward, const Vector3f& up)
{
    Matrix3f r;
    r.col(2) = forward.normalized();
    r.col(0) = up.cross(r.col(2)).normalized();
    r.col(1) = r.col(2).cross(r.col(0)).normalized();
    return r;
}

App::App(void)
{
    if (static_instance_ != 0)
        fail("Attempting to create a second instance of App!");

    static_instance_ = this;
}

void App::run()
{
    // Warn about cwd problems
    filesystem::path cwd = filesystem::current_path();
    if (!filesystem::is_directory(cwd / "assets")) {
        cout << fmt::format("Current working directory \"{}\" does not contain an \"assets\" folder.\n", cwd.string())
             << "Make sure the executable gets run relative to the project root.\n";
        return;
    }

    static_assert(is_standard_layout<VertexPNT>::value, "struct Vertex must be standard layout to use offsetof");

    // Initialize GLFW
    if (!glfwInit())
        fail("glfwInit() failed");

    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_REFRESH_RATE, 0);

    // Create a windowed mode window and its OpenGL context
    window_ = glfwCreateWindow(1920, 1080, "CS-C3100 Computer Graphics, Assignment 6", NULL, NULL);
    if (!window_)
    {
        glfwTerminate();
        fail("glfwCreateWindow() failed");
    }

    // Make the window's context current
    glfwMakeContextCurrent(window_);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(0);

    // Initialize ImGui and ImPlot
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    io_ = &ImGui::GetIO();
    io_->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io_->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Keep track of previous callbacks for chaining
    default_key_callback_ = glfwSetKeyCallback(window_, App::key_callback);
    default_mouse_button_callback_ = glfwSetMouseButtonCallback(window_, App::mousebutton_callback);
    default_cursor_pos_callback_ = glfwSetCursorPosCallback(window_, App::cursorpos_callback);
    default_drop_callback_ = glfwSetDropCallback(window_, App::drop_callback);
    default_scroll_callback_ = glfwSetScrollCallback(window_, App::scroll_callback);

    if (!Im3d_Init())
        fail("Error initializing Im3d!");

    initRendering();

    loadScene(filesystem::path("assets/meshes/head/head.txt"));

    setUIScale(1.5f);

    vector<string> vecStatusMessages;

    // MAIN LOOP
    while (!glfwWindowShouldClose(window_))
    {
        vecStatusMessages.clear();

        glfwPollEvents();

        // Rebuild font atlas if necessary
        if (font_atlas_dirty_)
        {
            io_->Fonts->Build();
            ImGui_ImplOpenGL3_DestroyFontsTexture();
            ImGui_ImplOpenGL3_CreateFontsTexture();
            font_atlas_dirty_ = false;
        }

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // For MacOS's weird display scaling
        auto s = io_->DisplayFramebufferScale;
        float xscale = s.x;
        float yscale = s.y;

        int width, height;
        const int gui_width = 400;
        glfwGetFramebufferSize(window_, &width, &height);
        int render_width = width - gui_width * xscale;

        ImGui::SetNextWindowPos(ImVec2(gui_width, 0));
        ImGui::SetNextWindowSize(ImVec2(render_width, height));
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Render surface", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);

        // Set render area window
        m_viewportX = gui_width * xscale;
        m_viewportY = 0;
        m_viewportW = render_width;
        m_viewportH = height;
        glViewport(m_viewportX, m_viewportY, m_viewportW, m_viewportH);

        // Clear screen.
        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if ( m_shaderProgram != nullptr )
            render(render_width, height, vecStatusMessages);
        else
        {
            float h = 0.0f;
            float w = 0.0f;
            for (const string& s : m_shaderCompilationErrors)
            {
                ImVec2 textSize = ImGui::CalcTextSize(s.c_str());
                w = max(w, textSize.x);
                h = h + textSize.y;
            }
            float line_start = render_width / 2.0f - w / 2.0f;
            ImGui::SetCursorPos(ImVec2(line_start, height / 2.0f - h / 2.0f));
            for (const string& s : m_shaderCompilationErrors)
            {
                ImGui::SetCursorPosX(line_start);
                ImGui::Text(s.c_str());
            }
        }

        ImGui::End();

        ImGui::PopStyleVar();

        // Begin GUI window
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(gui_width, height));
        ImGui::SetNextWindowBgAlpha(1.0f);
        if (ImGui::Begin("Controls", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
        {
			int width = 256;

			ImGui::SetNextItemWidth(width);
			ImGui::Checkbox("Render wireframe", &m_wireframe);
			ImGui::SetNextItemWidth(width);
			ImGui::Checkbox("Use diffuse texture", &m_useDiffuseTexture);
			ImGui::SetNextItemWidth(width);
			ImGui::Checkbox("Use normal map", &m_useNormalMap);
            ImGui::SetNextItemWidth(width);
            ImGui::Checkbox("Use shadow maps", &m_shadows);

            ImGui::SetNextItemWidth(96);
            ImGui::Checkbox("Override", &m_overrideRoughness);
            float roughness_slider = log10f(m_roughness);
            ImGui::SameLine(128);
            ImGui::SetNextItemWidth(128);
            ImGui::SliderFloat("Roughness", &roughness_slider, -4.0f, 0.0f, fmt::format("{:.3e}", m_roughness).c_str());
            m_roughness = float(pow(10.0, roughness_slider));

            if (ImGui::TreeNodeEx("Debug visualizations", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::SetNextItemWidth(width);
                if (ImGui::RadioButton("Mode 0 (full shader)", m_renderMode == 0))
                    m_renderMode = 0;
                ImGui::SetNextItemWidth(width);
                if (ImGui::RadioButton("Mode 1 (diffuse texture)", m_renderMode == 1))
                    m_renderMode = 1;
                ImGui::SetNextItemWidth(width);
                if (ImGui::RadioButton("Mode 2 (normal map texture)", m_renderMode == 2))
                    m_renderMode = 2;
                ImGui::SetNextItemWidth(width);
                if (ImGui::RadioButton("Mode 3 (final normal)", m_renderMode == 3))
                    m_renderMode = 3;
                ImGui::SetNextItemWidth(width);
                if (ImGui::RadioButton("Mode 4 (GGX normal distribution)", m_renderMode == 4))
                    m_renderMode = 4;
                ImGui::SetNextItemWidth(width);
                if (ImGui::RadioButton("Mode 5 (GGX geometry term)", m_renderMode == 5))
                    m_renderMode = 5;
                ImGui::SetNextItemWidth(width);
                if (ImGui::RadioButton("Mode 6 (Fresnel term)", m_renderMode == 6))
                    m_renderMode = 6;
                ImGui::TreePop();
            }

            if (ImGui::TreeNodeEx("Light & viewpoint control", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Camera");
                ImGui::SameLine(96);
                if (ImGui::RadioButton("View", m_viewpoint == 0))
                    m_viewpoint = 0;
                for (unsigned int i = 0; i < m_lights.size(); ++i)
                {
                    ImGui::Text(fmt::format("Light {}", i + 1).c_str());
                    ImGui::SameLine(96);
                    if (ImGui::RadioButton(fmt::format("View##{}", i).c_str(), m_viewpoint == i + 1))
                        m_viewpoint = i + 1;
                    ImGui::SameLine(96 + 96);
                    bool b = m_lights[i].first.isEnabled();
                    if (ImGui::Checkbox(fmt::format("Active##{}", i).c_str(), &b))
                        m_lights[i].first.setEnabled(b);
                }
                ImGui::Text("Sh. eps");
                ImGui::SameLine(96);
                ImGui::SliderFloat("##shadoweps", &m_shadowEps, -0.02f, 0.02f);
                ImGui::TreePop();
            }

            // Draw the status messages in the current list
            // Note that you can add them from inside render(...)
			//Matrix4f C = camera_.GetWorldToView().inverse();
			//vecStatusMessages.push_back(fmt::format("Camera at ({:.2f}, {:.2f}, {:.2f})\n looking at ({:.2f}, {:.2f}, {:.2f})",
			//	C(0, 3), C(1, 3), C(2, 3), C(0, 2), C(1, 2), C(2, 2)));
            vecStatusMessages.push_back("Mouse left + drag rotates");
            vecStatusMessages.push_back("Mouse right + drag zooms");
            vecStatusMessages.push_back("Mouse middle + drag translates");
            vecStatusMessages.push_back(fmt::format("Application average {:.3f} ms/frame ({:.1f} FPS)", 1000.0f / io_->Framerate, io_->Framerate));
            //vecStatusMessages.push_back("Home/End and mouse drag rotate camera");
            for (const string& msg : vecStatusMessages)
            {
                ImGui::SetNextItemWidth(width);
                ImGui::Text("%s", msg.c_str());
            }
        } // Begin("Controls")

        // end the GUI window (note: End() is always supposed to be called, regardless of what Begin(...) returned!)
        ImGui::End();

        // This creates the command buffer that is rendered below after our own drawing stuff
        ImGui::Render();

        // ..and this actually draws it using OpenGL
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // This shows the image that was just rendered in the window.
        glfwSwapBuffers(window_);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window_);
    glfwTerminate();
}

//------------------------------------------------------------------------

App::~App(void)
{
}

//------------------------------------------------------------------------

//void App::readState(StateDump& d)
//{
//    String meshFileName, rayDumpFileName;
//
//    d.pushOwner("App");
//    d.get(meshFileName,     "m_meshFileName");
//    d.get((S32&)m_cullMode, "m_cullMode");
//    d.get(m_wireframe,      "m_wireframe");
//    d.get(rayDumpFileName,  "m_rayDumpFileName");
//    d.popOwner();
//
//    if (m_meshFileName != meshFileName && meshFileName.getLength())
//        loadMesh(meshFileName);
//}

//------------------------------------------------------------------------

//void App::writeState(StateDump& d) const
//{
//    d.pushOwner("App");
//    d.set(m_meshFileName,       "m_meshFileName");
//    d.set((S32)m_cullMode,      "m_cullMode");
//    d.set(m_wireframe,          "m_wireframe");
//    d.set(m_rayDumpFileName,    "m_rayDumpFileName");
//    d.popOwner();
//}

//------------------------------------------------------------------------

void App::render(int width, int height, vector<string>& vecStatusMessages)
{
    // No mesh => skip.
    if (m_submeshes.empty())
    {
        vecStatusMessages.push_back("No scene loaded!");
        return;
    }

	// YOUR SHADOWS HERE: this is a good place to e.g. move the lights.
    for (auto& light : m_lights)
    {
        LightSource& l = light.first;
        l.setPosition(l.getStaticPosition());
        l.setOrientation(getOrientation(-l.getPosition(), Vector3f(0.0f, 1.0f, 0.0f)));
    }
        

	if (m_shadows)
	{
        glAssert(glBindVertexArray(gl_.vao));
        glAssert(glBindBuffer(GL_ARRAY_BUFFER, gl_.vertex_buffer));
        glAssert(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_.index_buffer));

        m_shadowMapShader->use();
		for (auto& lpair : m_lights)
		{
			auto& light = lpair.first;
			auto& shadowCtx = lpair.second;

			// Copy near and far distances from camera to light source
			light.setFar(100.0f);  // TODO
			light.setNear(0.1f);

			// Render light's shadow map
			light.renderShadowMap(m_indices.size(), m_shadowMapShader.get(), shadowCtx, m_boundingbox_min, m_boundingbox_max);
		}

        glAssert(glBindBuffer(GL_ARRAY_BUFFER, 0));
        glAssert(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
        glAssert(glBindVertexArray(0));

		// shadowmap sets its own viewport, so reset this back
		glViewport(m_viewportX, m_viewportY, m_viewportW, m_viewportH);
	}
    // Setup transformations.

    camera_.SetDimensions(width, height);
    camera_.SetViewport(0, 0, width, height);
    camera_.SetPerspective(50);

    Matrix4f worldToCamera = camera_.GetWorldToView();
    Matrix4f projection = camera_.GetProjection();

    double mousex, mousey;
    glfwGetCursorPos(window_, &mousex, &mousey);
    Im3d_NewFrame(window_, width, height, worldToCamera, projection, 0.0f, mousex, mousey);

    // Initialize GL state.

    glClearColor(0.2f, 0.4f, 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    if (m_cullMode == CullMode_None)
        glDisable(GL_CULL_FACE);
    else
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace((m_cullMode == CullMode_CW) ? GL_CCW : GL_CW);
    }

    glPolygonMode(GL_FRONT_AND_BACK, (m_wireframe) ? GL_LINE : GL_FILL);

    // Render.

	renderWithNormalMap(worldToCamera, projection);
	
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// visualize lights
	for (const auto& l : m_lights)
		l.first.visualize();

    // mark the coordinate origin
	//Im3d::BeginPoints();
	//Im3d::SetSize(8.0f);
	//Im3d::SetColor(Im3d::Color_Red);
	//Im3d::Vertex(0, 0, 0);
	//Im3d::End();
	
    Im3d_EndFrame();
}

void App::loadScene(const filesystem::path& scenefile)
{
    m_submeshes.clear();
	m_vertices.clear();
	m_indices.clear();

	m_boundingbox_min = Vector3f(FLT_MAX, FLT_MAX, FLT_MAX);
	m_boundingbox_max = Vector3f(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    auto ext = scenefile.extension();
	if (ext == ".obj")
	{
		// load obj files as normal
		loadMesh(scenefile);
		return;
	}
	else if (ext == ".txt")
	{
		// Get file path to use relative paths for obj files in scene file
        auto dir = scenefile.parent_path();

		// Clear lights list since the scene file will contain a new one
		m_lights.clear();

		// Open file for read
		ifstream filestream(scenefile);
		string line;

		// Read file line by line
		while (getline(filestream, line))
		{
            string keyword;
			stringstream stream(line);	// Stream for parsing line word by word
			stream >> keyword;

			if (keyword == "obj")
			{
				string name;
				stream >> name; // Read relative path of obj file
				loadMesh(dir / filesystem::path(name)); // Combine name to full path and load mesh
			}
			else if (keyword == "light")
			{
				// Add new entry to lights list and store reference to it so we can set its parameters
				m_lights.push_back(std::pair<LightSource, ShadowMapContext>());
				auto& light = m_lights.back().first;
				m_lights.back().second.setup(Vector2i(1024, 1024));

				// New loop to parse light data
				while (getline(filestream, line))
				{
					stringstream lightStream(line);
					lightStream >> keyword;

					if (keyword == "}")
						break;
					else if (keyword == "pos")
					{
						Vector3f p;
						lightStream >> p(0) >> p(1) >> p(2);
						light.setStaticPosition(p);
					}
					else if (keyword == "col")
					{
						lightStream >> light.m_color(0) >> light.m_color(1) >> light.m_color(2);
					}
				}
			}
		}
	}
	else
	{
		fail(fmt::format("Unknown file suffix in scene: {}", scenefile.string()));
	}

    // DEBUG: disable the final green light
    if (m_lights.size() > 2)
        for (int l = 2; l < m_lights.size(); ++l)
            m_lights[l].first.setEnabled(false);
}

void App::loadMesh(const filesystem::path& objfilename)
{
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = objfilename.parent_path().string(); // Path to material .mtl files = same path as .obj
    reader_config.triangulate = true;

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(objfilename.string(), reader_config))
    {
        if (!reader.Error().empty())
            fail(fmt::format("TinyObj Error: {}", reader.Error()));
    }

    if (!reader.Warning().empty())
        cout << "TinyObjReader: " << reader.Warning();

    // figure out how many materials are used by the shapes (these become individual meshes)
    set<int> used_materials;
    for (const auto& s : reader.GetShapes())
        for (const auto& t : s.mesh.material_ids)
            used_materials.insert(t);

	Vector3f bbmin(FLT_MAX, FLT_MAX, FLT_MAX);
	Vector3f bbmax(-bbmin);

	vector<vector<Vector3i>> indices(used_materials.size());
   
    // generate geometry, one mesh per material
    for (const auto& s : reader.GetShapes())
    {
        assert(s.mesh.indices.size() == 3 * s.mesh.material_ids.size());
        for (unsigned int i = 0; i < s.mesh.indices.size() / 3; ++i)
        {
			// add vertices, flatten indexing for simplicity
            for (unsigned int j = 0; j < 3; ++j)
            {
                VertexPNT v;
                v.position = *(Vector3f*)&reader.GetAttrib().vertices[3 * s.mesh.indices[i*3 + j].vertex_index];
                v.normal   = *(Vector3f*)&reader.GetAttrib().normals[3 * s.mesh.indices[i*3 + j].normal_index];
                v.uv       = *(Vector2f*)&reader.GetAttrib().texcoords[2 * s.mesh.indices[i*3 + j].texcoord_index];
				v.uv(1) = -v.uv(1); // the head mesh is using flipped coordinates.. gahh.
                m_vertices.push_back(v);
				bbmin = bbmin.cwiseMin(v.position);
				bbmax = bbmax.cwiseMax(v.position);
			}
			int vi = m_vertices.size() - 3;
			indices[s.mesh.material_ids[i]].push_back(Vector3i(vi, vi + 1, vi + 2));
        }
    }

	// allocate new meshes, assign their materials & index buffer offsets
	// then insert the submeshes' indices to the global list in sequential ordering
	// this allows the submeshes to index vertices in "out of order" but shouldn't matter.
	m_submeshes.resize(m_submeshes.size() + used_materials.size());
	for (unsigned int i = 0; i < used_materials.size(); ++i)
	{
		int submesh_idx = m_submeshes.size() - used_materials.size() + i;
		SubMesh& sm = m_submeshes[submesh_idx];
		sm.material = reader.GetMaterials()[i];
		sm.index_start = m_indices.size();
		sm.num_triangles = indices[i].size();
		m_indices.insert(m_indices.end(), indices[i].begin(), indices[i].end());
	}

	cout << fmt::format("Materials used: {}\n", used_materials.size());
	cout << fmt::format("Bounding box: ({:.2f}, {:.2f}, {:.2f}) - ({:.2f}, {:.2f}, {:.2f})\n", bbmin(0), bbmin(1), bbmin(2), bbmax(0), bbmax(1), bbmax(2));
	
	// update whole scene bounding box
	m_boundingbox_min = m_boundingbox_min.cwiseMin(bbmin);
	m_boundingbox_max = m_boundingbox_max.cwiseMax(bbmax);

	uploadToGPU(objfilename.parent_path());

	// convert normal map
	//{
	//	FILE* pF = fopen( "head/nm.raw", "rb" );
	//	fseek( pF, 0, SEEK_END );
	//	S64 size = ftell(pF);
	//	char* b = (char*)malloc( size );
	//	fseek( pF, 0, SEEK_SET );
	//	fread( b, size, 1, pF );
	//	fclose( pF );
	//	Image bump( Vec2i(6000,6000), ImageFormat::RGBA_Vec4f );
	//	for ( int i = 0; i < 6000*6000; ++i )
	//		bump.setVec4f( Vec2i(i%6000,i/6000), Vec4f(((unsigned short*)b)[i]/65535.f) );
	//	delete[] b;
	//	Image img = convertBumpToObjectSpaceNormal( bump, m_mesh, 0.0025f );
	//	exportImage( "nmap.png", &img );
	//	exit(0);
	//}

}

void App::uploadToGPU(const filesystem::path& texturepath)
{
	// vertex and index data
	glAssert(glBindBuffer(GL_ARRAY_BUFFER, gl_.vertex_buffer));
	glAssert(glBufferData(GL_ARRAY_BUFFER, sizeof(VertexPNT) * m_vertices.size(), m_vertices.data(), GL_STATIC_DRAW));
	glAssert(glBindBuffer(GL_ARRAY_BUFFER, 0));
	glAssert(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_.index_buffer));
	glAssert(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Vector3i) * m_indices.size(), m_indices.data(), GL_STATIC_DRAW));
	glAssert(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

	// kill current textures
	for (const auto& t : m_textures)
		glDeleteTextures(1, &t.second);
	m_textures.clear();

	// then load the new ones
	// first figure out what we have to load
	set<string> texturemaps;
	for (const auto& m : m_submeshes)
	{
		texturemaps.insert(m.material.diffuse_texname);
		texturemaps.insert(m.material.specular_texname);
		texturemaps.insert(m.material.bump_texname);
		texturemaps.insert(m.material.roughness_texname);
	}
	if (texturemaps.find("") != texturemaps.end())
		texturemaps.erase("");
	// then generate OpenGL texture ids and map names to them
	vector<GLuint> texIDs(texturemaps.size());
	glGenTextures(texturemaps.size(), texIDs.data());
	int cid = 0;
	for (const string& tname : texturemaps)
		m_textures[tname] = texIDs[cid++];
	// then load the actual contents
	for (const auto& name_id : m_textures)
	{
		Image4u8 teximg = Image4u8::loadPNG((texturepath / filesystem::path(name_id.first)).string());

		glBindTexture(GL_TEXTURE_2D, name_id.second);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Uncomment to enable anisotropic filtering: (not tested to work in 2024)
    	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, FW_S32_MAX);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
			teximg.getSize()(0), teximg.getSize()(1),
			0, GL_RGBA, GL_UNSIGNED_BYTE, teximg.data());

		//glGenerateTextureMipmap(name_id.second);
        glGenerateMipmap(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

//------------------------------------------------------------------------

void App::initRendering()
{
    // Create vertex attribute objects and buffers for vertex data.
    glAssert(glGenVertexArrays(1, &gl_.vao));
    glAssert(glGenBuffers(1, &gl_.vertex_buffer));
    glAssert(glGenBuffers(1, &gl_.index_buffer));

    int position_attribute = 0;
    int normal_attribute = 1;
    int uv_attribute = 2;

    // Set up vertex attribute object.
    glBindVertexArray(gl_.vao);
    glBindBuffer(GL_ARRAY_BUFFER, gl_.vertex_buffer);
    glEnableVertexAttribArray(position_attribute);
    glVertexAttribPointer(position_attribute, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPNT), (GLvoid*)0);
    glEnableVertexAttribArray(normal_attribute);
    glVertexAttribPointer(normal_attribute, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPNT), (GLvoid*)offsetof(VertexPNT, normal));
    glEnableVertexAttribArray(uv_attribute);
    glVertexAttribPointer(uv_attribute, 2, GL_FLOAT, GL_FALSE, sizeof(VertexPNT), (GLvoid*)offsetof(VertexPNT, uv));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    compileShadowMapShader();

    // Then set up OpenGL for indexed rendering

    m_vertex_shader_source =
        "#version 330\n"
        FW_GL_SHADER_SOURCE(
            uniform mat4 posToClip;
            uniform mat4 posToCamera;
            layout(location = 0) in vec3 positionAttrib;
            layout(location = 1) in vec3 normalAttrib;
            layout(location = 2) in vec2 texCoordAttrib;

            out vec3 positionVarying;
            out vec3 normalVarying;
            out vec3 tangentVarying;
            out vec2 texCoordVarying;
            out float lightDist[3];
            out vec2 shadowUV[3];

            uniform int numLights;
            uniform mat4 posToLightClip[3];
            uniform int renderFromLight;

            void main()
            {
                vec4 g = vec4(positionAttrib, 1.);
                gl_Position = posToClip * g;
                positionVarying = (posToCamera * g).xyz;
                normalVarying = normalAttrib;
                texCoordVarying = texCoordAttrib;
                tangentVarying = vec3(1, 0, 0) - normalAttrib.x * normalAttrib;
                for (int v = 0; v < numLights; v++)
                {
                    vec4 p = posToLightClip[v] * g;
                    lightDist[v] = p.z / p.w;
                    shadowUV[v] = p.xy / p.w * .5 + .5;
                }
                if (renderFromLight > 0)
                    gl_Position = posToLightClip[renderFromLight - 1] * g;
            }
        );

    string pixel_shader_source = loadPixelShader();

    if ( pixel_shader_source != "" )
        loadAndCompileShaders(m_vertex_shader_source, pixel_shader_source);
}

string App::loadPixelShader()
{
    m_shaderProgram.reset();
    m_shaderCompilationErrors.clear();
    string shader_file = "shaders/pixel_shader.glsl";
    auto psf = ifstream(shader_file, ios::in);
    auto b = psf.is_open();
    if (!b)
    {
        m_shaderCompilationErrors.push_back(fmt::format("Could not open {}", shader_file));
        m_shaderCompilationErrors.push_back("(Is your working directory set correctly?)");
        return "";
    }
    psf.seekg(0, ios::end);
    auto l = psf.tellg();
    string ps(l, '\0');
    psf.seekg(0, ios::beg);
    psf.read(ps.data(), ps.size());
    return ps;
}

void App::loadAndCompileShaders(const string& vs, const string& ps)
{
	ShaderProgram* pProgram = nullptr;
	try
	{
        pProgram = new ShaderProgram(vs, ps);
	}
	catch ( ShaderProgram::ShaderCompilationException& e )
	{
        m_shaderCompilationErrors.clear();
        char* msg = const_cast<char*>(e.msg_.c_str());
        char* line = strtok(msg, "\n");;
        while(line)
        {
            m_shaderCompilationErrors.push_back(string(line));
            line = strtok(nullptr, "\n");
        }
        return;
	}

    m_shaderProgram.reset(pProgram);
	gl_.shader = pProgram->getHandle();

	// enumerate and print uniforms used by the compiled shader (for debugging)
	GLint numUniforms;
	glGetProgramiv(gl_.shader, GL_ACTIVE_UNIFORMS, &numUniforms);
	for (GLint i = 0; i < numUniforms; ++i) {
		char name[256];
		GLsizei length;
		GLint size;
		GLenum type;
		glGetActiveUniform(gl_.shader, i, sizeof(name), &length, &size, &type, name);
		string typestr = GetGLTypeString(type);
		GLint location = glGetUniformLocation(gl_.shader, name);

		// Print or process uniform information
		cout << fmt::format("Uniform #{}: name = {}, type = {} ({}), size = {}, location = {}\n", i, name, typestr, type, size, location);
	}

	// do the same for input attributes
	GLint numAttributes;
	glGetProgramiv(gl_.shader, GL_ACTIVE_ATTRIBUTES, &numAttributes);
	for (GLint i = 0; i < numAttributes; ++i) {
		char name[256];
		GLsizei length;
		GLint size;
		GLenum type;
		glGetActiveAttrib(gl_.shader, i, sizeof(name), &length, &size, &type, name);
		string typestr = GetGLTypeString(type);
		// Get the location of the attribute
		GLint location = glGetAttribLocation(gl_.shader, name);

		m_vertex_attribute_indices[name] = location;

		// Print or process attribute information
		cout << fmt::format("Attribute #{}: name = {}, type = {} ({}), size = {}, location = {}\n", i, name, typestr, type, size, location);
	}
}

void App::compileShadowMapShader()
{
    // Get or build the shader that outputs depth values
    ShaderProgram* prog = new ShaderProgram(
            "#version 330\n"
            FW_GL_SHADER_SOURCE(
            // Have a look at the comments in the main shader in the renderShadowedScene() function in case
            // you are unfamiliar with GLSL shaders.

            // This is the shader that will render the scene from the light source's point of view, and
            // output the depth value at every pixel.

            // VERTEX SHADER

            // The posToLightClip uniform will be used to transform the world coordinate points of the scene.
            // They should end up in the clip coordinates of the light source "camera". If you look below,
            // you'll see that its value is set based on the function getWorldToLightClip(), which you'll
            // need to implement.
            uniform mat4 posToLightClip;
            uniform mat4 posToLight;
            layout(location = 0) in vec3 positionAttrib;
            out float depthVarying;

    void main()
    {
        // Here we perform the transformation from world to light clip space.
        // If the posToLightClip is set correctly (see the corresponding setUniform()
        // call below), the scene will be viewed from the light.
        vec4 pos = vec4(positionAttrib, 1.0);
        gl_Position = posToLightClip * pos;
        // YOUR SHADOWS HERE: compute the depth of this vertex and assign it to depthVarying. Note that
        // there are various alternative "depths" you can store: there is at least (A) the z-coordinate in camera
        // space, (B) z-coordinate in clip space divided by the w-coordinate.
        // All of these work, but you must use the same convention also in the main shader that actually
        // reads from the depth map. We used the option B in the example solution.

        depthVarying = gl_Position.z / gl_Position.w;
    }
        ),
        "#version 330\n"
        FW_GL_SHADER_SOURCE(
            // FRAGMENT SHADER
            in float depthVarying;
            out vec4 fragColor;
        void main()
        {
            // YOUR SHADOWS HERE
            // Just output the depth value you computed in the vertex shader. It'll go into the shadow
            // map texture.
            fragColor = vec4(depthVarying);
        }
        ));

	gl_.shadowMapShader = prog->getHandle();
    m_shadowMapShader.reset(prog);
}

void App::renderWithNormalMap(const Matrix4f& worldToCamera, const Matrix4f& projection)
{
	if (m_wireframe)
	{
		Im3d::BeginLines();
		Im3d::SetSize(1.0f);
		Im3d::SetColor(Im3d::Color_White);
		for (const Vector3i& t : m_indices)
			for (unsigned int j = 0; j < 3; ++j)
			{
				Im3d::Vertex(m_vertices[t[j]].position);
				Im3d::Vertex(m_vertices[t[(j + 1) % 3]].position);
			}
		Im3d::End();
		return;
	}

    // Setup uniforms.

    m_shaderProgram->use();
	m_shaderProgram->setUniform("posToClip", Matrix4f(projection * worldToCamera));
	m_shaderProgram->setUniform("posToCamera", worldToCamera);
	Matrix3f normalToCamera = Matrix3f(Matrix3f(Matrix3f(worldToCamera.block(0, 0, 3, 3)).inverse()).transpose());
	m_shaderProgram->setUniform("normalToCamera", normalToCamera);
    m_shaderProgram->setUniform("diffuseSampler", 0);
	m_shaderProgram->setUniform("specularSampler", 0); // we don't actually have a specular albedo texture for the head so this will have to do
    m_shaderProgram->setUniform("normalSampler", 1);

	vector<Vector3f> lightDir, I;
	for (auto& lpair : m_lights)
	{
        // transform light direction to camera space
        lightDir.push_back(worldToCamera.block(0, 0, 3, 3) * lpair.first.getPosition().normalized());
		I.push_back(lpair.first.isEnabled() ? lpair.first.m_color : Vector3f::Zero());
	}

	m_shaderProgram->setUniformArray("lightDirections", m_lights.size(), lightDir.data());
	m_shaderProgram->setUniformArray("lightIntensities", m_lights.size(), I.data());
	m_shaderProgram->setUniform("numLights", (int)m_lights.size());

	m_shaderProgram->setUniform("renderMode", m_renderMode);
	m_shaderProgram->setUniform("normalMapScale", m_normalMapScale);
	m_shaderProgram->setUniform("setDiffuseToZero", m_setDiffuseToZero);
	m_shaderProgram->setUniform("setSpecularToZero", m_setSpecularToZero);
	m_shaderProgram->setUniform("shadowMaps", m_shadows);

	m_shaderProgram->setUniformArray("shadowSampler", 3, array<int, 3>{0, 0, 0}.data());
	m_shaderProgram->setUniform("renderFromLight", 0);

	glAssert(glBindVertexArray(gl_.vao));
	glAssert(glBindBuffer(GL_ARRAY_BUFFER, gl_.vertex_buffer));
	glAssert(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_.index_buffer));

	// Shadow maps
	static vector<Matrix4f> lightMatrices;
	lightMatrices.clear();
    static vector<GLint> shadowSamplers;
    shadowSamplers.clear();
	for (unsigned i = 0; i < m_lights.size(); i++)
	{
		LightSource& ls = m_lights[i].first;
        Matrix4f m = ls.getPosToLightClip(m_boundingbox_min, m_boundingbox_max);
		lightMatrices.push_back(m);
        glActiveTexture(GL_TEXTURE2 + i);
        glBindTexture(GL_TEXTURE_2D, ls.getShadowTextureHandle());
        shadowSamplers.push_back(2 + i);
	}
	m_shaderProgram->setUniformArray("posToLightClip", 3, lightMatrices.data());
    m_shaderProgram->setUniformArray("shadowSampler", 3, shadowSamplers.data());

	m_shaderProgram->setUniform("renderFromLight", m_viewpoint);
    m_shaderProgram->setUniform("shadowMaps", m_shadows);
    m_shaderProgram->setUniform("shadowEps", m_shadowEps);

	// render all meshes
	for (const auto& mesh : m_submeshes)
	{
		const tinyobj::material_t& mat = mesh.material;

	    m_shaderProgram->setUniform("diffuseUniform", Vector4f(mat.diffuse));
        float roughness = sqrt(2.0f / (mat.shininess + 2.0f));
        if (m_overrideRoughness)
            roughness = m_roughness;
	    m_shaderProgram->setUniform("roughness", roughness);

		bool useDiffuseTexture = !mat.diffuse_texname.empty() && m_useDiffuseTexture && m_renderMode < 4;
		m_shaderProgram->setUniform("useTextures", useDiffuseTexture);
		if (useDiffuseTexture)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, m_textures[mat.diffuse_texname]);
		}

		bool useNormalMap = !mat.bump_texname.empty() && m_useNormalMap;
		m_shaderProgram->setUniform("useNormalMap", useNormalMap);
		if (useNormalMap)
		{
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, m_textures[mat.bump_texname]);
		}

		glAssert(glDrawElements(GL_TRIANGLES, (GLsizei)3 * mesh.num_triangles, GL_UNSIGNED_INT, (void*)(3 * (int64_t)mesh.index_start)));
	}


	glAssert(glUseProgram(0));
	glAssert(glBindVertexArray(0));
	glAssert(glBindBuffer(GL_ARRAY_BUFFER, 0));
	glAssert(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

 //   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_mesh->getVBO().getGLBuffer());
 //   m_mesh->setGLAttrib(gl, posAttrib, m_shaderProgram->getAttribLoc("positionAttrib"));

 //   if (normalAttrib != -1)
 //       m_mesh->setGLAttrib(gl, normalAttrib, m_shaderProgram->getAttribLoc("normalAttrib"));
 //   else if ( m_shaderProgram->getAttribLoc("normalAttrib") != -1 )
 //       glVertexAttrib3f(m_shaderProgram->getAttribLoc("normalAttrib"), 0.0f, 0.0f, 0.0f);

 //   if (vcolorAttrib != -1)
 //       m_mesh->setGLAttrib(gl, vcolorAttrib, m_shaderProgram->getAttribLoc("vcolorAttrib"));
 //   else if ( m_shaderProgram->getAttribLoc("vcolorAttrib") != -1 )
 //       glVertexAttrib4f(m_shaderProgram->getAttribLoc("vcolorAttrib"), 1.0f, 1.0f, 1.0f, 1.0f);

 //   if (texCoordAttrib != -1)
 //       m_mesh->setGLAttrib(gl, texCoordAttrib, m_shaderProgram->getAttribLoc("texCoordAttrib"));
 //   else if ( m_shaderProgram->getAttribLoc("texCoordAttrib") != -1 )
 //       glVertexAttrib2f(m_shaderProgram->getAttribLoc("texCoordAttrib"), 0.0f, 0.0f);

 //   // Render each submesh.

	//gl->setUniform(m_shaderProgram->getUniformLoc("renderMode"), m_renderMode );
	//gl->setUniform(m_shaderProgram->getUniformLoc("normalMapScale"), m_normalMapScale );
	//gl->setUniform(m_shaderProgram->getUniformLoc("setDiffuseToZero"), m_setDiffuseToZero );
	//gl->setUniform(m_shaderProgram->getUniformLoc("setSpecularToZero"), m_setSpecularToZero );

	//// Shadow maps
	//for (unsigned i = 0; i < m_lights.size(); i++) {
	//	LightSource& ls = m_lights[i].first;
	//	// Each array item is also a single uniform in opengl
	//	String ss = "shadowSampler["; ss += ('0' + (char)i); ss += ']';
	//	String ptlc = "posToLightClip["; ptlc += ('0' + (char)i); ptlc += ']';
	//	glActiveTexture(GL_TEXTURE2 + i);
	//	glBindTexture(GL_TEXTURE_2D, ls.getShadowTextureHandle());
	//	gl->setUniform(m_shaderProgram->getUniformLoc(ss.getPtr()), int(2 + i));
	//	gl->setUniform(m_shaderProgram->getUniformLoc(ptlc.getPtr()), ls.getPosToLightClip());
	//}
	//gl->setUniform(m_shaderProgram->getUniformLoc("renderFromLight"), m_viewpoint);
	//gl->setUniform(m_shaderProgram->getUniformLoc("shadowMaps"), m_shadowMode != 0);
	//gl->setUniform(m_shaderProgram->getUniformLoc("shadowEps"), m_shadowEps);

	//for (int i = 0; i < m_mesh->numSubmeshes(); i++)
 //   {
 //       const MeshBase::Material& mat = m_mesh->material(i);
 //       gl->setUniform(m_shaderProgram->getUniformLoc("diffuseUniform"), mat.diffuse);
 //       gl->setUniform(m_shaderProgram->getUniformLoc("roughness"), sqrt(2.0f / (mat.glossiness + 2.0f)));

 //       glActiveTexture(GL_TEXTURE0);
 //       glBindTexture(GL_TEXTURE_2D, mat.textures[MeshBase::TextureType_Diffuse].getGLTexture());
 //       gl->setUniform(m_shaderProgram->getUniformLoc("useTextures"), mat.textures[MeshBase::TextureType_Diffuse].exists() && m_useDiffuseTexture && m_renderMode<4);

 //       glActiveTexture(GL_TEXTURE1);
	//	glBindTexture(GL_TEXTURE_2D, mat.textures[MeshBase::TextureType_Normal].getGLTexture());
	//	gl->setUniform(m_shaderProgram->getUniformLoc("useNormalMap"), mat.textures[MeshBase::TextureType_Normal].exists() && m_useNormalMap);

 //       glDrawElements(GL_TRIANGLES, m_mesh->vboIndexSize(i), GL_UNSIGNED_INT, (void*)(UPTR)m_mesh->vboIndexOffset(i));
 //   }

 //   gl->resetAttribs();
}

Image3f App::convertBumpToObjectSpaceNormal(const Image1f& bump, void* pMeshIn, float alpha)
{
	Vector2i s = bump.getSize();
	int W = s(0);
	int H = s(1);

	// result normal map
	Image3f nmap(s, Vector3f::Zero());

	//// convert into suitable format 
	//Mesh<VertexPNT>* pMesh = new Mesh<VertexPNT>( *pMeshIn );

	//// loop over triangles, rasterize into normal map texture
	//for ( int m = 0; m < pMesh->numSubmeshes(); ++m )
	//	for ( int t = 0; t < pMesh->indices(m).getSize(); ++t )
	//	{
	//		// get vertices' UV coordinates
	//		Vec2f v[3];
	//		v[0] = pMesh->vertex(pMesh->indices(m)[t].x).t;
	//		v[1] = pMesh->vertex(pMesh->indices(m)[t].y).t;
	//		v[2] = pMesh->vertex(pMesh->indices(m)[t].z).t;
	//		v[0].x *= W; v[0].y *= H;
	//		v[1].x *= W; v[1].y *= H;
	//		v[2].x *= W; v[2].y *= H;

	//		// for getting barys from UVs, as per rasterization lecture slides
	//		Mat3f interpolation;
	//		interpolation.setCol( 0, Vec3f( v[0], 1.0f ) );
	//		interpolation.setCol( 1, Vec3f( v[1], 1.0f ) );
	//		interpolation.setCol( 2, Vec3f( v[2], 1.0f ) );
	//		interpolation.invert();

	//		Mat3f PM;
	//		PM.setCol( 0, pMesh->vertex(pMesh->indices(m)[t].x).p );
	//		PM.setCol( 1, pMesh->vertex(pMesh->indices(m)[t].y).p );
	//		PM.setCol( 2, pMesh->vertex(pMesh->indices(m)[t].z).p );
	//		Mat3f NM;
	//		NM.setCol( 0, pMesh->vertex(pMesh->indices(m)[t].x).n );
	//		NM.setCol( 1, pMesh->vertex(pMesh->indices(m)[t].y).n );
	//		NM.setCol( 2, pMesh->vertex(pMesh->indices(m)[t].z).n );

	//		Mat3f Mp = PM * interpolation;	// Mp = vertex matrix * inv(uv matrix), gives point on mesh as function of x, y
	//		Mat3f Mn = NM * interpolation;	// Mn = normal matrix * inv(uv matrix), gives normal as function of x, y

	//		// rasterize in fixed point, 8 bits of subpixel precision
	//		// first convert vertices to FP
	//		S64 v_[6]; 
	//		v_[0] = (S64)(v[0].x*256); v_[1] = (S64)(v[0].y*256);
	//		v_[2] = (S64)(v[1].x*256); v_[3] = (S64)(v[1].y*256);
	//		v_[4] = (S64)(v[2].x*256); v_[5] = (S64)(v[2].y*256);

	//		// compute fixed-point edge functions
	//		S64 edges[3][3];
	//		edges[0][0] = v_[3]-v_[1]; edges[0][1] = -(v_[2]-v_[0]); edges[0][2] = -(edges[0][0]*v_[0] + edges[0][1]*v_[1]);
	//		edges[1][0] = v_[5]-v_[3]; edges[1][1] = -(v_[4]-v_[2]); edges[1][2] = -(edges[1][0]*v_[2] + edges[1][1]*v_[3]);
	//		edges[2][0] = v_[1]-v_[5]; edges[2][1] = -(v_[0]-v_[4]); edges[2][2] = -(edges[2][0]*v_[4] + edges[2][1]*v_[5]);

	//		Vec2i bbmin, bbmax;
	//		// bounding box minimum: just chop off fractional part
	//		bbmin = Vec2i( int(min(v_[0], v_[2], v_[4]) >> 8), int(min(v_[1], v_[3], v_[5]) >> 8) );
	//		// maximum: have to round up
	//		bbmax = Vec2i( (max(v_[0], v_[2], v_[4]) & 0xFF) ? 1 : 0, (max(v_[1], v_[3], v_[5]) & 0xFF) ? 1 : 0 );
	//		bbmax += Vec2i( int(max(v_[0], v_[2], v_[4]) >> 8), int(max(v_[1], v_[3], v_[5]) >> 8) );

	//		for ( int y = bbmin.y; y <= bbmax.y; ++y )
	//			for ( int x = bbmin.x; x <= bbmax.x; ++x )
	//			{
	//				// center of pixel
	//				S64 px = (x << 8) + 127;
	//				S64 py = (y << 8) + 127;

	//				// evaluate edge functions
	//				S64 e[3];
	//				e[0] = px*edges[0][0] + py*edges[0][1] + edges[0][2];
	//				e[1] = px*edges[1][0] + py*edges[1][1] + edges[1][2];
	//				e[2] = px*edges[2][0] + py*edges[2][1] + edges[2][2];

	//				// test for containment
	//				if ( (e[0] >= 0 && e[1] >= 0 && e[2] >= 0) ||
	//					 (e[0] <= 0 && e[1] <= 0 && e[2] <= 0) )
	//				{
	//					// we are in, bet barys
	//					Vec3f b = interpolation * Vec3f( x+0.5f, y+0.5f, 1.0f );
	//					b = b * (1.0f / b.sum());	// should already sum to one but what the heck

	//					// interpolate normal on mesh surface, NOT normalized
	//					Vec3f N = b[0] * pMesh->vertex(pMesh->indices(m)[t].x).n + 
	//						      b[1] * pMesh->vertex(pMesh->indices(m)[t].y).n +
	//							  b[2] * pMesh->vertex(pMesh->indices(m)[t].z).n;
	//					N.normalize();

	//					// compute finite difference derivative of height
	//					//float f00 = bump.getVec4f( Vec2i(x,   y )).x;
	//					float f01 = bump.getVec4f( Vec2i(x-1, y) ).x;
	//					float f02 = bump.getVec4f( Vec2i(x+1, y )).x;
	//					float f03 = bump.getVec4f( Vec2i(x,   y-1 )).x;
	//					float f04 = bump.getVec4f( Vec2i(x,   y+1 )).x;

	//					// central differencing to get dh/du, dh/dv
	//					float dhdx = (f02-f01) / 2.0f;
	//					float dhdy = (f04-f03) / 2.0f;

	//					//     P' = P(u,v) + n(u,v) + \alpha * n(u,v) * h(u,v)
	//					// =>  dP'/du = dP/du + dn/du + \alpha*[ h*dn/du + n*dh/du ]
	//					// dP/du = Mp(:,0), dP/dv = Mp(:,1) (Matlab notation)
	//					// similar for dn/du,v

	//					Vec3f dPdx = Mp.getCol(0);
	//					Vec3f dPdy = Mp.getCol(1);
	//					float l1 = dPdx.length();
	//					float l2 = dPdy.length();
	//					dPdx -= N*dot(dPdx,N);
	//					dPdy -= N*dot(dPdy,N);
	//					dPdx = dPdx.normalized()*l1;
	//					dPdy = dPdy.normalized()*l2;

	//					Vec3f dndx = Mn.getCol(0);
	//					Vec3f dndy = Mn.getCol(1);

	//					//Vec3f dndu = Mn.getCol(0) / N.length();
	//					//dndu -= dot(Mn.getCol(0), N) * (1.0f / pow(dot(N,N),1.5f));
	//					//Vec3f dndv = Mn.getCol(1) / N.length();
	//					//dndv -= dot(Mn.getCol(1), N) * (1.0f / pow(dot(N,N),1.5f));
	//					//dndu *= (1.0f / W);
	//					//dndv *= (1.0f / H);
	//					//dPdu *= (1.0f / W);
	//					//dPdv *= (1.0f / H);

	//					//Vec3f dPpdx = dPdx + alpha * (dndx*f00 + dhdx*N);
	//					//Vec3f dPpdy = dPdy + alpha * (dndy*f00 + dhdy*N);
	//					Vec3f dPpdx = dPdx + alpha * (dhdx*N);
	//					Vec3f dPpdy = dPdy + alpha * (dhdy*N);

	//					// ha!
	//					Vec3f bumpedNormal = cross( dPpdx, dPpdy ).normalized();

	//					if ( dot( bumpedNormal, N ) < 0.0f )
	//						bumpedNormal = -bumpedNormal;

	//					//bumpedNormal = 10.0f * Vec3f( dhdx, dhdy, 0.0f );

	//					nmap.setVec4f( Vec2i(x,y), Vec4f( bumpedNormal*0.5+0.5, 0.0f ) );
	//					//nmap.setVec4f( Vec2i(x,y), bump.getVec4f(Vec2i(x,y)) );						
	//					//nmap.setVec4f( Vec2i(x,y), Vec4f( N.normalized()*0.5+0.5, 1.0f ) );
	//				}
	//			}
	//	}

	//delete pMesh;

	return nmap;
}

void App::handleDrop(GLFWwindow* window, int count, const char** paths) {}
void App::handleKeypress(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
        case GLFW_KEY_0:
            m_renderMode = 0;
            break;
        case GLFW_KEY_1:
            m_renderMode = 1;
            break;
        case GLFW_KEY_2:
            m_renderMode = 2;
            break;
        case GLFW_KEY_3:
            m_renderMode = 3;
            break;
        case GLFW_KEY_4:
            m_renderMode = 4;
            break;
        case GLFW_KEY_5:
            m_renderMode = 5;
            break;
        case GLFW_KEY_6:
            m_renderMode = 6;
            break;
        case GLFW_KEY_F5:
            {
                string ps = loadPixelShader();
                if ( ps != "" )
                    loadAndCompileShaders(m_vertex_shader_source, ps);
            }
            break;
        }
    }
    else if (action == GLFW_RELEASE)
    {
    }
}

void App::handleMouseButton(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        camera_.MouseClick(button, x, y);
    }
    else if (action == GLFW_RELEASE)
    {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        camera_.MouseRelease(x, y);
    }
}

void App::handleMouseMovement(GLFWwindow* window, double xpos, double ypos)
{
    // if the user is interacting with an Im3d gizmo, don't do anything else
    if (Im3d::GetContext().m_activeId != 0)
        return;

    camera_.MouseDrag(xpos, ypos);
}

void App::handleScroll(GLFWwindow* window, double xoffset, double yoffset)
{
}

void App::loadFont(const char* name, float sizePixels)
{
    string fontPathRel = fmt::format("assets/fonts/{}", name); // relative path: only ascii
    string fontPathAbs = (filesystem::current_path() / fontPathRel).string();
    if (!filesystem::exists(fontPathAbs)) {
        fail(fmt::format("Error: Could not open font file \"{}\"\n", fontPathAbs));
    }

    io_->Fonts->Clear();
    font_ = io_->Fonts->AddFontFromFileTTF(fontPathRel.c_str(), sizePixels); // imgui doesn't like non-ascii
    assert(font_ != nullptr);
}

//------------------------------------------------------------------------

void App::increaseUIScale()
{
    setUIScale(ui_scale_ * 1.1);
}

//------------------------------------------------------------------------

void App::decreaseUIScale()
{
    setUIScale(ui_scale_ / 1.1);
}

//------------------------------------------------------------------------

void App::setUIScale(float scale)
{
    ui_scale_ = scale;
    loadFont(CS3100_TTF_PATH, 14.0f * ui_scale_);
    font_atlas_dirty_ = true;
}

//------------------------------------------------------------------------
// Static function, does not have access to member functions except through static_instance_-> ...

void App::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // First check for exit
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    // Always forward events to ImGui
    default_key_callback_(window, key, scancode, action, mods);

    //// Already handled?
    //if (ImGui::GetIO().WantCaptureKeyboard)
    //    return;

    if (!ImGui::IsAnyItemActive())
        static_instance_->handleKeypress(window, key, scancode, action, mods);
}

//------------------------------------------------------------------------
// Static function, does not have access to member functions except through static_instance_-> ...

void App::mousebutton_callback(GLFWwindow* window, int button, int action, int mods)
{
    // Always forward events to ImGui
    default_mouse_button_callback_(window, button, action, mods);

    // Already handled?
    if (ImGui::GetIO().WantCaptureMouse)
        return;

    static_instance_->handleMouseButton(window, button, action, mods);
}

//------------------------------------------------------------------------
// Static function, does not have access to member functions except through static_instance_-> ...

void App::cursorpos_callback(GLFWwindow* window, double xpos, double ypos)
{
    // Always forward events to ImGui
    default_cursor_pos_callback_(window, xpos, ypos);

    // Already handled?
    if (ImGui::GetIO().WantCaptureMouse)
        return;

    static_instance_->handleMouseMovement(window, xpos, ypos);
}

//------------------------------------------------------------------------
// Static function, does not have access to member functions except through static_instance_-> ...

void App::drop_callback(GLFWwindow* window, int count, const char** paths)
{
    static_instance_->handleDrop(window, count, paths);
}

//------------------------------------------------------------------------
// Static function, does not have access to member functions except through static_instance_-> ...

void App::error_callback(int error, const char* description)
{
    fail(fmt::format("Error {}: {}\n", error, description));
}

//------------------------------------------------------------------------
// Static function, does not have access to member functions except through static_instance_-> ...

void App::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Always forward events to ImGui
    default_scroll_callback_(window, xoffset, yoffset);

    // Already handled?
    if (ImGui::GetIO().WantCaptureMouse)
        return;

    static_instance_->handleScroll(window, xoffset, yoffset);
}
