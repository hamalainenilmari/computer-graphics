#define _USE_MATH_DEFINES

#include "app.h"
#include "parse.h"
#include "subdiv.h"

#include <fmt/core.h>

#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include "im3d.h"

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

//------------------------------------------------------------------------

// defined in im3d_opengl33.cpp
bool Im3d_Init();
void Im3d_NewFrame(int width, int height, const Matrix4f& model_to_clip);
void Im3d_EndFrame();
namespace Im3d
{
    inline void Vertex(const Vector3f& v) { Im3d::Vertex(v(0), v(1), v(2)); }
    inline void Vertex(const Vector4f& v) { Im3d::Vertex(v(0), v(1), v(2), v(3)); }
}

//------------------------------------------------------------------------

App::App(void)
{
    if (static_instance_ != 0)
        fail("Attempting to create a second instance of App!");

    static_instance_ = this;
}

//------------------------------------------------------------------------

App::~App()
{
    static_instance_ = 0;
}

//------------------------------------------------------------------------

void App::run()
{
    // Warn about cwd problems
    filesystem::path cwd = filesystem::current_path();
    if (!filesystem::is_directory(cwd/"assets")) {
        cout << fmt::format("Current working directory \"{}\" does not contain an \"assets\" folder.\n", cwd.string())
             << "Make sure the executable gets run relative to the project root.\n";
    }
    
    // Initialize GLFW
    if (!glfwInit()) {
        fail("glfwInit() failed");
    }

    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_REFRESH_RATE, 0);

    // Create a windowed mode window and its OpenGL context
    window_ = glfwCreateWindow(1920, 1080, "CS-C3100 Computer Graphics, Assignment 2", NULL, NULL);
    if (!window_) {
        glfwTerminate();
        fail("glfwCreateWindow() failed");
    }

    // Make the window's context current
    glfwMakeContextCurrent(window_);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(0);

    auto err = glGetError();

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
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

    if (!Im3d_Init())
        fail("Error initializing Im3d!");

    err = glGetError();

    // generate vertex buffer objects, load shaders, etc.
    initRendering();

    err = glGetError();

    // also loads corresponding font
    setUIScale(1.5f);

    vector<string> vecStatusMessages;

    // MAIN LOOP
    while (!glfwWindowShouldClose(window_))
    {
        // This vector holds the strings that are printed out.
        // It's also passed to render(...) so you can output
        // your own debug information from there if you like.
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

        // First, render our own 3D scene using OpenGL
        int width, height;
        glfwGetFramebufferSize(window_, &width, &height);
        render(width, height, vecStatusMessages);

        // Begin GUI window
        ImGui::Begin("Controls");

        if (ImGui::RadioButton("Curve Mode (1)", draw_mode_ == DrawMode_Curves))
            draw_mode_ = DrawMode_Curves;
        if (ImGui::RadioButton("Subdivision Mode (2)", draw_mode_ == DrawMode_Subdivision))
            draw_mode_ = DrawMode_Subdivision;

        if (draw_mode_ == DrawMode_Curves)
        {
            if (ImGui::Button("Load SWP curve file (L)"))
                handleLoading();
            if (ImGui::SliderInt("Tessellation steps", &tessellation_steps_, 1, 32))
                tessellateCurves();
        }

        if (draw_mode_ == DrawMode_Subdivision)
        {
            if (ImGui::Button("Load OBJ mesh (L)"))
                handleLoading();
            float mid = 200.0f * ui_scale_;
            if ( ImGui::Button("Increase subdivision (KP+)") )
                increaseSubdivisionLevel();
            ImGui::SameLine(mid);
            if (ImGui::Button("Decrease subdivision (KP-)"))
                decreaseSubdivisionLevel();

            ImGui::Checkbox("Render wireframe (W)", &draw_wireframe_);
            ImGui::Checkbox("Show connectivity (D)", &debug_subdivision_);
        }

        // Draw the status messages in the current list
        // Note that you can add them from inside render(...)
        vecStatusMessages.push_back(fmt::format("Application average {:.3f} ms/frame ({:.1f} FPS)", 1000.0f / io_->Framerate, io_->Framerate));
        for (const string& msg : vecStatusMessages)
            ImGui::Text("%s", msg.c_str());

        // end the GUI window
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
    ImGui::DestroyContext();

    glfwDestroyWindow(window_);
    glfwTerminate();
}

//------------------------------------------------------------------------

// w, h - width and height of the window in pixels.
void App::setupViewportAndProjection(int w, int h)
{
    camera_.SetDimensions(w, h);
    camera_.SetViewport(0, 0, w, h);
    camera_.SetPerspective(50);
}

//------------------------------------------------------------------------

// This function is responsible for displaying the object.
void App::render(int window_width, int window_height, vector<string>& vecStatusMessages)
{
    // Remove any shader that may be in use.
    glUseProgram(0);

    // Clear the rendering window
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // tell OpenGL the size of the surface we're drawing into
    glViewport(0, 0, window_width, window_height);

    // this sets the camera's internal state
    setupViewportAndProjection(window_width, window_height);

    Matrix4f model_to_clip = camera_.GetPerspective() * camera_.GetModelview();
    Im3d_NewFrame(window_width, window_height, model_to_clip);

    switch(draw_mode_)
    {
    case DrawMode_Curves:
            renderCurves();
            break;

    case DrawMode_Subdivision:
        if (subdivided_meshes_.size() > 0)
        {
            const MeshWithConnectivity& m = *subdivided_meshes_[current_subdivision_level_];

            // check if mouse is on top of a triangle and show debug info if requested
            int highlight_triangle = -1;
            if (debug_subdivision_)
            {
                double mx, my;
                glfwGetCursorPos(window_, &mx, &my);
                ImVec2 fbScale = ImGui::GetIO().DisplayFramebufferScale; // Mac Retina specific
                highlight_triangle = pickTriangle(m, camera_, window_width, window_height, mx * fbScale.x, my * fbScale.y);

                vecStatusMessages.push_back(fmt::format("Selected triangle: {}", highlight_triangle));
                if (highlight_triangle != -1)
                {
                    const Vector3i& i = m.indices[highlight_triangle];
                    const Vector3i& nt = m.neighborTris[highlight_triangle];
                    const Vector3i& ne = m.neighborEdges[highlight_triangle];
                    vecStatusMessages.push_back(fmt::format("             Indices: {:3d}, {:3d}, {:3d}", i(0), i(1), i(2)));
                    vecStatusMessages.push_back(fmt::format("  Neighbor triangles: {:3d}, {:3d}, {:3d}", nt(0), nt(1), nt(2)));
                    vecStatusMessages.push_back(fmt::format("      Neighbor edges: {:3d}, {:3d}, {:3d}", ne(0), ne(1), ne(2)));
                }
            }

            renderMesh(m, camera_, draw_wireframe_, highlight_triangle);
        }
        break;
	}

    // this draws the accumulated buffers 
    Im3d_EndFrame();
}

void App::renderMesh(const MeshWithConnectivity& m, const Camera& cam, bool include_wireframe, int highlight_triangle) const
{
    Matrix4f world_to_view = cam.GetModelview();
    Matrix4f view_to_clip = cam.GetPerspective();
    Matrix4f view_to_world = world_to_view.inverse();
    Vector3f camera_position = view_to_world.block(0, 3, 3, 1);

    glUseProgram(gl_.shader_program);
    auto err = glGetError();
    glUniform1f(gl_.shading_toggle_uniform, true ? 1.0f : 0.0f);
    err = glGetError();
    glUniformMatrix4fv(gl_.view_to_clip_uniform, 1, GL_FALSE, view_to_clip.data());
    err = glGetError();
    glUniformMatrix4fv(gl_.world_to_view_uniform, 1, GL_FALSE, world_to_view.data());
    err = glGetError();
    glUniform3fv(gl_.camera_world_position_uniform, 1, camera_position.data());
    err = glGetError();


    // Draw the model with your model-to-world transformation.
    glBindVertexArray(gl_.vao);
    glBindBuffer(GL_ARRAY_BUFFER, gl_.vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_.index_buffer);
    glDrawElements(GL_TRIANGLES, (GLsizei)3 * m.indices.size(), GL_UNSIGNED_INT, 0);

    // Undo our bindings.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(0);

    if (include_wireframe || (highlight_triangle != -1 && debug_subdivision_))
    {
        static vector<size_t> only_highlighted_triangle({0});
        static vector<size_t> all_triangles;
        if (all_triangles.size() != m.indices.size())
        {
            all_triangles.resize(m.indices.size());
            for (size_t i = 0; i < m.indices.size(); ++i)
                all_triangles[i] = i;
        }
        const vector<size_t>* index_list = &all_triangles;
        if (!include_wireframe && highlight_triangle != -1 && debug_subdivision_)
        {
            only_highlighted_triangle[0] = highlight_triangle;
            index_list = &only_highlighted_triangle;
        }

        Im3d::BeginLines();
        for (size_t j = 0; j < index_list->size(); ++j)
        {
            size_t i = (*index_list)[j];

            const Vector3i& f = m.indices[i];

            if (i == highlight_triangle)
                Im3d::SetSize(8.0f);
            else
                Im3d::SetSize(2.0f);

            // prepare a slightly smaller triangle so that we can code its edges with colors
            auto v0 = m.positions[f[0]];
            auto v1 = m.positions[f[1]];
            auto v2 = m.positions[f[2]];
            auto n0 = m.normals[f[0]];
            auto n1 = m.normals[f[1]];
            auto n2 = m.normals[f[2]];
            Vector3f tn = (v1 - v0).cross(v2 - v0).normalized() * 0.01f;
            Vector3f c = (v0 + v1 + v2) / 3;
            const float t = 0.95f;
            Vector3f nv0 = t * v0 + (1.0f - t) * c + tn;
            Vector3f nv1 = t * v1 + (1.0f - t) * c + tn;
            Vector3f nv2 = t * v2 + (1.0f - t) * c + tn;

            Im3d::SetColor(1.0f, 0.0f, 0.0f);   // 1st edge: red
            Im3d::Vertex(nv0);
            Im3d::Vertex(nv1);
            Im3d::SetColor(0.0f, 1.0f, 0.0f);   // 2nd edge: green
            Im3d::Vertex(nv1);
            Im3d::Vertex(nv2);
            Im3d::SetColor(0.0f, 0.0f, 1.0f);   // 3rd edge: blue
            Im3d::Vertex(nv2);
            Im3d::Vertex(nv0);
        }
        Im3d::End();
    }
}

//------------------------------------------------------------------------

// Initialize OpenGL's rendering modes
void App::initRendering()
{
    glEnable(GL_DEPTH_TEST);   // Depth testing must be turned on

    // Create vertex attribute objects and buffers for vertex and index data.
    glGenVertexArrays(1, &gl_.vao);
    glGenBuffers(1, &gl_.vertex_buffer);
    glGenBuffers(1, &gl_.index_buffer);

    // Set up vertex attribute object
    glBindVertexArray(gl_.vao);
    glBindBuffer(GL_ARRAY_BUFFER, gl_.vertex_buffer);
    glEnableVertexAttribArray(ATTRIB_POSITION);
    glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPNC), (GLvoid*)0);
    glEnableVertexAttribArray(ATTRIB_NORMAL);
    glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPNC), (GLvoid*)offsetof(VertexPNC, normal));
    glEnableVertexAttribArray(ATTRIB_COLOR);
    glVertexAttribPointer(ATTRIB_COLOR, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPNC), (GLvoid*)offsetof(VertexPNC, color));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_.index_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_.index_buffer);
    glBindVertexArray(0);


    auto shader_program = new ShaderProgram(
        "#version 330\n"
        "#extension GL_ARB_separate_shader_objects : enable\n"
        FW_GL_SHADER_SOURCE(
            layout(location = 0) in vec4 aPosition;
            layout(location = 1) in vec3 aNormal;
            layout(location = 2) in vec4 aColor;

            layout(location = 0) out vec3 vWorldPos;
            layout(location = 1) out vec3 vNormal;
            layout(location = 2) out vec4 vColor;

            uniform mat4 uWorldToView;
            uniform mat4 uViewToClip;
            uniform float uShading;

            void main()
            {
                gl_Position = uViewToClip * uWorldToView * aPosition;
                vNormal = aNormal;
                vColor = aColor;
                vWorldPos = aPosition.xyz;
            }
        ),
        "#version 330\n"
        "#extension GL_ARB_separate_shader_objects : enable\n"
        FW_GL_SHADER_SOURCE(
            layout(location = 0) in vec3 vWorldPos;
            layout(location = 1) in vec3 vNormal;
            layout(location = 2) in vec4 vColor;

            uniform vec3 uCameraWorldPosition;  // not used right now

            const vec3 cLightDirection1 = normalize(vec3(0.5, 0.5, 0.6));
            const vec3 cLightDirection2 = normalize(vec3(-1, 0, 0));
            const vec3 cLightColor1 = vec3(1, 1, 1);
            const vec3 cLightColor2 = vec3(0.4, 0.3, 0.4);

            out vec4 fColor;

            void main()
            {
                vec3 n = normalize(vNormal);
                vec3 l1 = cLightDirection1;
                vec3 l2 = cLightDirection2;

                float ndotl1 = clamp(dot(n, l1), 0, 1);
                float ndotl2 = clamp(dot(n, l2), 0, 1);

                vec3 shading = vColor.xyz * (ndotl1*cLightColor1 + ndotl2*cLightColor2);     // diffuse

                fColor = vec4(shading, vColor.a);
            }
        ));

    // Get the IDs of the shader program and its uniform input locations from OpenGL.
    gl_.shader_program = shader_program->getHandle();
    gl_.view_to_clip_uniform = glGetUniformLocation(gl_.shader_program, "uViewToClip");
    gl_.world_to_view_uniform = glGetUniformLocation(gl_.shader_program, "uWorldToView");
    gl_.shading_toggle_uniform = glGetUniformLocation(gl_.shader_program, "uShading");
    gl_.camera_world_position_uniform = glGetUniformLocation(gl_.shader_program, "uCameraWorldPosition");
}

//------------------------------------------------------------------------

// Load in objects from standard input into the class member variables: 
// spline_curves_, tessellated_curves_, curve_names_, surfaces_, m_surfaceNames.  If
// loading fails, this will exit the program.
void App::loadSWP(const string& filename)
{
    cout << endl << "*** loading and constructing curves and surfaces ***" << endl;
	
	//int pathEnd = filename.find_last_of("/\\");
	//string path = filename.substr(0, pathEnd + 1);

    if (!parseSWP(filename, spline_curves_)) {
        cerr << "\aerror in file format\a" << endl;
        exit(-1);
    }

    cerr << endl << "*** done ***" << endl;
}

//------------------------------------------------------------------------

void App::loadOBJ(const string& filename)
{
    // get rid of the old meshes, if any
    subdivided_meshes_.clear();
    current_subdivision_level_ = 0;

    auto pNewMesh = MeshWithConnectivity::loadOBJ(filename);

    // hoist it into GPU memory...
    uploadGeometryToGPU(*pNewMesh);

    // and write result down for later use.
    subdivided_meshes_.push_back(unique_ptr<MeshWithConnectivity>(pNewMesh));
}

//------------------------------------------------------------------------

void App::increaseSubdivisionLevel()
{
    ++current_subdivision_level_;
    // compute new mesh if we haven't done that already
    // (also, we need to have the initial model loaded)
    if (!subdivided_meshes_.empty() && current_subdivision_level_ >= subdivided_meshes_.size())
    {
        // copy constuct finest mesh
        MeshWithConnectivity* pNewMesh = new MeshWithConnectivity(*subdivided_meshes_.back());
        pNewMesh->LoopSubdivision();
        pNewMesh->computeConnectivity();
        pNewMesh->computeVertexNormals();
        subdivided_meshes_.push_back(unique_ptr<MeshWithConnectivity>(pNewMesh));
    }
    uploadGeometryToGPU(*subdivided_meshes_[current_subdivision_level_]);
}

//------------------------------------------------------------------------

void App::decreaseSubdivisionLevel()
{
    current_subdivision_level_ = max(0, current_subdivision_level_ - 1);
    uploadGeometryToGPU(*subdivided_meshes_[current_subdivision_level_]);
}

//------------------------------------------------------------------------

void App::uploadGeometryToGPU(const MeshWithConnectivity& m)
{
    static vector<VertexPNC> v;
    v.resize(m.positions.size());
    for (size_t i = 0; i < v.size(); ++i)
    {
        v[i].position = m.positions[i];
        v[i].color = m.colors[i];
        v[i].normal = m.normals[i];
    }

    // Load the vertex buffer to GPU.
    glBindBuffer(GL_ARRAY_BUFFER, gl_.vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VertexPNC) * v.size(), v.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_.index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Vector3i) * m.indices.size(), m.indices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

//------------------------------------------------------------------------

void MeshWithConnectivity::computeVertexNormals()
{
    // Calculate average normal for each vertex position by
    // looping over triangles, computing their normals,
    // and adding them to each of its vertices' normal.

    typedef map<Vector3f, Vector3f, CompareVector3f> postonormal_t;
    postonormal_t posToNormal;
    Vector3f v[3];

    for (size_t j = 0; j < indices.size(); j++)
    {
        const Vector3i& tri = indices[j];
        for (int k = 0; k < 3; k++)
            v[k] = positions[tri[k]];

        Vector3f triNormal = (v[1] - v[0]).cross(v[2] - v[0]);
        for (int k = 0; k < 3; k++)
        {
            if (posToNormal.find(v[k]) != posToNormal.end())
                posToNormal[v[k]] += triNormal;
            else
                posToNormal[v[k]] = triNormal;
        }
    }

    // Output normals. Normalization yields the average of
    // the normals of all the triangles that share each vertex.
    for ( size_t i = 0; i < positions.size(); ++i )
    {
        if (posToNormal.find(positions[i]) != posToNormal.end())
            normals[i] = posToNormal[positions[i]].normalized();
    }
}

//------------------------------------------------------------------------

void App::writeObjects(const string& prefix) {
    cerr << endl << "*** writing obj files ***" << endl;

    //for (auto i = 0u; i < surface_names_.size(); ++i) {
    //    if (surface_names_[i] != ".") {
    //        string filename = prefix + "_" + surface_names_[i] + ".obj";
    //        ofstream out(filename);
    //        if (!out) {
    //            cerr << "\acould not open file " << filename << ", skipping"<< endl;
    //            out.close();
    //            continue;
    //        } else {
    //            outputObjFile(out, surfaces_[i]);
    //            cerr << "wrote " << filename <<  endl;
    //        }
    //    }
    //}
}

//------------------------------------------------------------------------

void App::tessellateCurves()
{
    tessellated_curves_.resize(spline_curves_.size());
    for (size_t i = 0; i < spline_curves_.size(); ++i)
    {
        switch (spline_curves_[i].curve_type)
        {
        case CurveType_Bezier:
            tessellateBezier(spline_curves_[i].control_points, tessellated_curves_[i], tessellation_steps_);
            break;
        case CurveType_BSpline:
            tessellateBspline(spline_curves_[i].control_points, tessellated_curves_[i], tessellation_steps_);
            break;
        }
    }
}

//------------------------------------------------------------------------

void App::renderCurves()
{    
    // draw the tessellated curves
    Im3d::SetColor(1.0f, 1.0f, 1.0f);
    Im3d::SetSize(2.0f);
    for (size_t i = 0; i < tessellated_curves_.size(); ++i)
        drawCurve(tessellated_curves_[i]);

    // draw control point sequences
    for (auto i = 0u; i < spline_curves_.size(); ++i)
    {
        const SplineCurve& c = spline_curves_[i];

        switch (c.curve_type)
        {
        case CurveType_Bezier:
            Im3d::SetColor(1.0f, 1.0f, 0.0f);
            break;
        case CurveType_BSpline:
            Im3d::SetColor(0.0f, 1.0f, 0.0f);
            break;
        }

        // draw larger points
        Im3d::PushSize(16.0f);
        Im3d::BeginPoints();
        for (const auto& cpt : c.control_points)
            Im3d::Vertex(cpt(0), cpt(1), cpt(2));
        Im3d::End();
        Im3d::PopSize();

        Im3d::BeginLineStrip();
        for (const auto& cpt : c.control_points)
            Im3d::Vertex(cpt(0), cpt(1), cpt(2));
        Im3d::End();
    }
}

int App::pickTriangle(const MeshWithConnectivity& m, const Camera& cam, int window_width, int window_height, float mousex, float mousey) const
{
    Matrix4f view_to_world = cam.GetModelview().inverse();
    Matrix4f clip_to_view = cam.GetPerspective().inverse();
    Vector3f o = view_to_world.block(0, 3, 3, 1); // ray origin

    Vector4f clipxy{  2.0f * float(mousex) / window_width - 1.0f,
                     -2.0f * float(mousey) / window_height + 1.0f,
                      1.0f,
                      1.0f };

    Vector4f e4 = view_to_world * clip_to_view * clipxy;    // end point
    Vector3f d = e4({ 0, 1, 2 }) / e4(3);   // project

    Im3d::BeginLineStrip();
    Im3d::Vertex(o);
    Im3d::Vertex(d);
    Im3d::End();

    d -= o;

    return m.pickTriangle(o, d);
}

void App::screenshot (const string& name) {
    //// Capture image.
    //const Vec2i& size = window_.getGL()->getViewSize();
    //Image image(size, ImageFormat::R8_G8_B8_A8);
    //glUseProgram(0);
    //glWindowPos2i(0, 0);
    //glReadPixels(0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, image.getMutablePtr());

    //// Display the captured image immediately.
    //for (int i = 0; i < 3; ++i) {
    //    glDrawPixels(size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, image.getPtr());
    //    window_.getGL()->swapBuffers();
    //}
    //glDrawPixels(size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, image.getPtr());

    //// Export.
    //image.flipY();
    //exportImage(name, &image);
    //printf("Saved screenshot to '%s'", name.getPtr());
}

//------------------------------------------------------------------------

void App::handleDrop(GLFWwindow* window, int count, const char** paths) {}
void App::handleKeypress(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        if (key == GLFW_KEY_O)
            decreaseUIScale();
        else if (key == GLFW_KEY_P)
            increaseUIScale();
        else if (key == GLFW_KEY_W)
            draw_wireframe_ = !draw_wireframe_;
        else if (key == GLFW_KEY_D)
            debug_subdivision_ = !debug_subdivision_;
        else if (key == GLFW_KEY_1)
            draw_mode_ = DrawMode_Curves;
        else if (key == GLFW_KEY_2)
            draw_mode_ = DrawMode_Subdivision;
        else if (key == GLFW_KEY_KP_ADD)
            increaseSubdivisionLevel();
        else if (key == GLFW_KEY_KP_SUBTRACT)
            decreaseSubdivisionLevel();
        else if (key == GLFW_KEY_L)
            handleLoading();
    }
}

void App::handleLoading()
{
    if (draw_mode_ == DrawMode_Curves)
    {
        string filename = fileOpenDialog("SWP curve specification file", "swp");
        if (filename != "")
        {
            loadSWP(filename);
            tessellateCurves();
        }
    }
    else if (draw_mode_ == DrawMode_Subdivision)
    {
        string filename = fileOpenDialog("OBJ mesh file", "obj");
        if (filename != "")
            loadOBJ(filename);
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
    camera_.MouseDrag(xpos, ypos);
}

void App::loadFont(const char* name, float sizePixels)
{
    string fontPath = (filesystem::current_path() / "assets" / "fonts" / name).string();
    if (!filesystem::exists(fontPath)) {
        fail(fmt::format("Error: Could not open font file \"{}\"\n", fontPath));
    }

    io_->Fonts->Clear();
    font_ = io_->Fonts->AddFontFromFileTTF(fontPath.c_str(), sizePixels);
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

    // Already handled?
    if (ImGui::GetIO().WantCaptureKeyboard)
        return;

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

void App::error_callback(int error, const char* description)
{
    fail(fmt::format("Error {}: {}\n", error, description));
}
