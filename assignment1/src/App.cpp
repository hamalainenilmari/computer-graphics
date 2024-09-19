
#include "App.h"
#include "Utils.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <fmt/core.h>

//------------------------------------------------------------------------

using namespace std;

//------------------------------------------------------------------------
// Static data members

App* App::static_instance_ = 0; // The single instance we allow of App

GLFWkeyfun          App::default_key_callback_ = nullptr;
GLFWmousebuttonfun  App::default_mouse_button_callback_ = nullptr;
GLFWcursorposfun    App::default_cursor_pos_callback_ = nullptr;
GLFWdropfun         App::default_drop_callback_ = nullptr;

enum VertexShaderAttributeLocations {
    ATTRIB_POSITION = 0,
    ATTRIB_NORMAL = 1,
    ATTRIB_COLOR = 2
};

const App::Vertex reference_plane_data[] = {
    { Vector3f(-1, -1, -1), Vector3f(0, 1, 0) },
    { Vector3f(1, -1, -1), Vector3f(0, 1, 0) },
    { Vector3f(1, -1,  1), Vector3f(0, 1, 0) },
    { Vector3f(-1, -1, -1), Vector3f(0, 1, 0) },
    { Vector3f(1, -1,  1), Vector3f(0, 1, 0) },
    { Vector3f(-1, -1,  1), Vector3f(0, 1, 0) }
};


//------------------------------------------------------------------------

App::App(void)
    : 
    shading_toggle_(false),
    camera_rotation_angle_(0.0f)
{
    if (static_instance_ != 0)
        fail("Attempting to create a second instance of App!");

    static_instance_ = this;

    static_assert(is_standard_layout<Vertex>::value, "struct Vertex must be standard layout to use offsetof");
}

//------------------------------------------------------------------------

App::~App()
{
    static_instance_ = 0;
}

//------------------------------------------------------------------------

void App::run()
{
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
    window_ = glfwCreateWindow(1920, 1080, "CS-C3100 Computer Graphics, Assignment 1", NULL, NULL);
    if (!window_) {
        glfwTerminate();
        fail("glfwCreateWindow() failed");
    }

    // Make the window's context current
    glfwMakeContextCurrent(window_);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(0);

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

    // generate vertex buffer objects, load shaders, etc.
    initRendering();

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
        vecStatusMessages.push_back("Use arrow keys, PgUp/PgDn to move the model (R1), Home/End to rotate camera.");

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
        // Model switching UI buttons
        if (ImGui::Button("Load Example Model"))
            uploadGeometryToGPU(loadExampleModel());
        ImGui::SameLine(ui_scale_ * 150.f);
        if (ImGui::Button("Load Indexed Model"))
            uploadGeometryToGPU(loadIndexedDataModel());
        if (ImGui::Button("Load Generated Cone"))
            uploadGeometryToGPU(loadGeneratedConeModel());
        ImGui::SameLine(ui_scale_ * 150.f);
        if (ImGui::Button("Load OBJ model (L)"))
            showObjLoadDialog();
        ImGui::Checkbox("Shading mode (S)", &shading_toggle_);

        // Draw the status messages in the current list
        // Note that you can add them from inside render(...)
        vecStatusMessages.push_back(fmt::format("Application average {:.3f} ms/frame ({:.1f} FPS)", 1000.0f / io_->Framerate, io_->Framerate));
        for (const string& msg : vecStatusMessages)
            ImGui::Text(msg.c_str());

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

void App::render(int window_width, int window_height, vector<string>& vecStatusMessages)
{
    // Clear screen.
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Enable depth testing.
    glEnable(GL_DEPTH_TEST);

    // Tell OpenGL the size of the buffer we're rendering into.
    glViewport(0, 0, window_width, window_height);

    // Set up a matrix to transform from world space to clip space.
    // Clip space is a [-1, 1]^3 space where OpenGL expects things to be
    // when it starts drawing them.
    // We piece the transformation together by first constructing
    // a mapping from the camera to world space, inverting it to get
    // a world-to-camera mapping and following that by the camera-to-clip projection.

    // Our camera is aimed at origin, and orbits around origin at fixed distance.
    static const float camera_distance = 2.1f;
    Matrix4f camera_to_world(Matrix4f::Identity());
    Matrix3f camera_orientation = Matrix3f(AngleAxis<float>(-camera_rotation_angle_, Vector3f(0, 1, 0)));
    camera_to_world.block(0, 0, 3, 3) = camera_orientation.inverse();
    camera_to_world.block(0, 3, 3, 1) = camera_orientation.inverse() * Vector3f { 0.0f, 0.0f, -camera_distance };;

    // Simple perspective.
    float aspect = float(window_width) / float(window_height);
    static const float fnear = 0.1f, ffar = 4.0f;

    // Construct projection matrix (mapping from camera to clip space).
    Matrix4f camera_to_clip(Matrix4f::Identity());
    camera_to_clip(0, 0) = min(1.f / aspect, 1.0f);
    camera_to_clip(1, 1) = min(aspect, 1.0f);
    camera_to_clip.col(2) = Vector4f(0, 0, (ffar + fnear) / (ffar - fnear), 1);
    camera_to_clip.col(3) = Vector4f(0, 0, -2 * ffar * fnear / (ffar - fnear), 0);

    Matrix4f world_to_clip = camera_to_clip * camera_to_world.inverse();

    // Set active shader program.
    glUseProgram(gl_.shader_program);
    glUniform1f(gl_.shading_toggle_uniform, shading_toggle_ ? 1.0f : 0.0f);
    glUniformMatrix4fv(gl_.world_to_clip_uniform, 1, GL_FALSE, world_to_clip.data());

    // Draw the reference plane. It is already in world coordinates.
    Matrix4f identity = Matrix4f::Identity();
    glUniformMatrix4fv(gl_.model_to_world_uniform, 1, GL_FALSE, identity.data());
    glBindVertexArray(gl_.static_vao);
    glDrawArrays(GL_TRIANGLES, 0, SIZEOF_ARRAY(reference_plane_data));

    // YOUR CODE HERE (R1)
    // Set the model space -> world space transform to translate the model according to user input.
    Matrix4f modelToWorld = Matrix4f::Identity();
    modelToWorld.block<3, 1>(0, 3) = currentTranslation;

    // Draw the model with your model-to-world transformation.
    glUniformMatrix4fv(gl_.model_to_world_uniform, 1, GL_FALSE, modelToWorld.data());
    glBindVertexArray(gl_.dynamic_vao);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertex_count_);

    // Undo our bindings.
    glBindVertexArray(0);
    glUseProgram(0);

    // Show status messages. You may find it useful to show some debug information in a message.
    Vector3f camPos = camera_to_world.block(0, 3, 3, 1);
    vecStatusMessages.push_back(fmt::format("Camera is at ({:.2f} {:.2f} {:.2f}) looking towards origin.",
        camPos(0), camPos(1), camPos(2)));
}

//------------------------------------------------------------------------

void App::handleKeypress(GLFWwindow * window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT )
    {
        // YOUR CODE HERE (R1)
        // React to user input and move the model.
        // See https://www.glfw.org/docs/3.4/group__keys.html for more key codes.
        // Visual Studio tip: you can right-click an identifier like GLFW_KEY_HOME
        // and "Go to definition" to jump directly to where the identifier is defined.
        if (key == GLFW_KEY_O)
            decreaseUIScale();
        else if (key == GLFW_KEY_P)
            increaseUIScale();
        else if (key == GLFW_KEY_HOME)
            camera_rotation_angle_ -= 0.05 * EIGEN_PI;
        else if (key == GLFW_KEY_END)
            camera_rotation_angle_ += 0.05 * EIGEN_PI;
        else if (key == GLFW_KEY_PAGE_UP)
            currentTranslation.z() += 0.05;
        else if (key == GLFW_KEY_PAGE_DOWN)
            currentTranslation.z() -= 0.05;
        else if (key == GLFW_KEY_UP)
            currentTranslation.y() += 0.05;
        else if (key == GLFW_KEY_DOWN)
            currentTranslation.y() -= 0.05;
        else if (key == GLFW_KEY_LEFT)
            currentTranslation.x() -= 0.05;
        else if (key == GLFW_KEY_RIGHT)
            currentTranslation.x() += 0.05;
    }
}

//------------------------------------------------------------------------

void App::handleMouseButton(GLFWwindow* window, int button, int action, int mods)
{
    // EXTRA: you can put your mouse controls here.
    // See GLFW Input documentation for details.
    if (action == GLFW_PRESS)
    {
    }
    else if (action == GLFW_RELEASE)
    {
    }
}

void App::handleMouseMovement(GLFWwindow* window, double xpos, double ypos)
{
    // EXTRA: you can put your mouse controls here.
    // See GLFW Input documentation for details.
}

//------------------------------------------------------------------------

void App::handleDrop(GLFWwindow* window, int count, const char** paths)
{
    // only look at the last thing to be dropped
    if (strlen(paths[count-1]) > 0)
    {
        string f(paths[count - 1]);
        for (unsigned int i = 0; i < f.length(); ++i)
            f[i] = tolower(f[i]);

        if (filesystem::path(f).extension() == ".obj")
            uploadGeometryToGPU(loadObjFileModel(paths[count-1]));  // notice we are using the original input string on purpose!
    }
}

//------------------------------------------------------------------------

void App::showObjLoadDialog()
{
    string filename = fileOpenDialog("OBJ model file", "obj");
    if (filename != "")
        uploadGeometryToGPU(loadObjFileModel(filename));
}

vector<App::Vertex> App::loadExampleModel()
{
    static const Vertex example_data[] = {
        { Vector3f(0.0f,  0.5f, 0), Vector3f(0.0f, 0.0f, -1.0f) },
        { Vector3f(-0.5f, -0.5f, 0), Vector3f(0.0f, 0.0f, -1.0f) },
        { Vector3f(0.5f, -0.5f, 0), Vector3f(0.0f, 0.0f, -1.0f) }
    };
    vector<Vertex> vertices;
    for (auto const& v : example_data)
        vertices.push_back(v);
    return vertices;
}

vector<App::Vertex> App::unpackIndexedData(
    const vector<Vector3f>& positions,
    const vector<Vector3f>& normals,
    const vector<array<unsigned, 6>>& faces)
{
    vector<Vertex> vertices;

    // This is a 'range-for' loop which goes through all objects in the container 'faces'.
    // '&' gives us a reference to the object inside the container; if we omitted '&',
    // 'f' would be a copy of the object instead.
    // The compiler already knows the type of objects inside the container, so we can
    // just write 'auto' instead of having to spell out 'array<unsigned, 6>'.
    for (auto& f : faces) {

        // YOUR CODE HERE (R3)
        // Unpack the indexed data into a vertex array. For every face, you have to
        // create three vertices and add them to the vector 'vertices'.

        // f[0] is the index of the position of the first vertex
        // f[1] is the index of the normal of the first vertex
        // f[2] is the index of the position of the second vertex
        // ...
        Vertex v0 = Vertex{ positions[f[0]], normals[f[1]] };
        Vertex v1 = Vertex{ positions[f[2]], normals[f[3]] };
        Vertex v2 = Vertex{ positions[f[4]], normals[f[5]] };
        vertices.push_back(v0);
        vertices.push_back(v1);
        vertices.push_back(v2);
    
    }

    return vertices;
};

// This is for testing your unpackIndexedData implementation.
// You should get a tetrahedron like in example.exe.
vector<App::Vertex> App::loadIndexedDataModel() {
    static const Vector3f point_data[] = {
        Vector3f(0.0f, 0.407f, 0.0f),
        Vector3f(0.0f, -0.3f, -0.5f),
        Vector3f(0.433f, -0.3f, 0.25f),
        Vector3f(-0.433f, -0.3f, 0.25f),
    };
    static const Vector3f normal_data[] = {
        Vector3f(0.8165f, 0.3334f, -0.4714f),
        Vector3f(0.0f, 0.3334f, 0.9428f),
        Vector3f(-0.8165f, 0.3334f, -0.4714f),
        Vector3f(0.0f, -1.0f, 0.0f)
    };
    static const unsigned face_data[][6] = {
        {0, 0, 1, 0, 2, 0},
        {0, 2, 3, 2, 1, 2},
        {0, 1, 2, 1, 3, 1},
        {1, 3, 3, 3, 2, 3}
    };
    vector<Vector3f> points(point_data, point_data + SIZEOF_ARRAY(point_data));
    vector<Vector3f> normals(normal_data, normal_data + SIZEOF_ARRAY(normal_data));
    vector<array<unsigned, 6>> faces;
    for (auto arr : face_data) {
        array<unsigned, 6> f;
        copy(arr, arr + 6, f.begin());
        faces.push_back(f);
    }
    return unpackIndexedData(points, normals, faces);
}

// Generate an upright cone with tip at (0, 0, 0), a radius of 0.25 and a height of 1.0.
// You can leave the base of the cone open, like it is in example.exe.
vector<App::Vertex> App::loadGeneratedConeModel()
{
    static const float radius = 0.25f;
    static const float height = 1.0f;
    static const unsigned faces = 40;
    static const float angle_increment = 2 * EIGEN_PI / faces;

    // Empty array of Vertex structs; every three vertices = one triangle
    vector<Vertex> vertices;

    Vertex v0(Vertex::Zero());
    Vertex v1(Vertex::Zero());
    Vertex v2(Vertex::Zero());

    // Generate one face at a time
    for (auto i = 0u; i < faces; ++i) {
        // YOUR CODE HERE (R2)
        // Figure out the correct positions of the three vertices of this face.
        v0.position = Vector3f(
            cos(angle_increment * i) * radius, // cos for x coordinate on a circle
            -1.0f,                             // height is the same
            sin(angle_increment * i) * radius  // sin for y coordinate on a circle
        );
        v1.position = Vector3f(
            cos(angle_increment * (i + 1)) * radius,
            -1.0f,
            sin(angle_increment * (i + 1)) * radius
        );
        v2.position = Vector3f(0.0f, 0.0f, 0.0f);
        // Calculate the normal of the face from the positions and use it for all vertices.
        v0.normal = v1.normal = v2.normal =
            ((v1.position - v0.position).cross(v2.position - v0.position)).normalized();
        //
        // Some hints:
        // - Try just making a triangle in fixed coordinates at first.
        // - "FW::cos(angle_increment * i) * radius" gives you the X-coordinate
        //    of the ith vertex at the base of the cone. Z-coordinate is very similar.
        // - For the normal calculation, you'll want to use the cross() function for
        //   cross product, and Vector3f's .normalized() or .normalize() methods.

        // Then we add the vertices to the array.
        // .push_back() grows the size of the vector by one, copies its argument,
        // and places the copy at the back of the vector.
        vertices.push_back(v0); vertices.push_back(v1); vertices.push_back(v2);
    }
    return vertices;
}

void App::uploadGeometryToGPU(const std::vector<Vertex>& vertices) {
    // Load the vertex buffer to GPU.
    glBindBuffer(GL_ARRAY_BUFFER, gl_.dynamic_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
    vertex_count_ = vertices.size();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void App::initRendering()
{
    // Create vertex attribute objects and buffers for vertex data.
    glGenVertexArrays(1, &gl_.static_vao);
    glGenVertexArrays(1, &gl_.dynamic_vao);
    glGenBuffers(1, &gl_.static_vertex_buffer);
    glGenBuffers(1, &gl_.dynamic_vertex_buffer);

    // Set up vertex attribute object for static data.
    glBindVertexArray(gl_.static_vao);
    glBindBuffer(GL_ARRAY_BUFFER, gl_.static_vertex_buffer);
    glEnableVertexAttribArray(ATTRIB_POSITION);
    glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
    glEnableVertexAttribArray(ATTRIB_NORMAL);
    glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, normal));

    // Load the static data to the GPU; needs to be done only once.
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * SIZEOF_ARRAY(reference_plane_data), reference_plane_data, GL_STATIC_DRAW);

    // Set up vertex attribute object for dynamic data. We'll load the actual data later, whenever the model changes.
    glBindVertexArray(gl_.dynamic_vao);
    glBindBuffer(GL_ARRAY_BUFFER, gl_.dynamic_vertex_buffer);
    glEnableVertexAttribArray(ATTRIB_POSITION);
    glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
    glEnableVertexAttribArray(ATTRIB_NORMAL);
    glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, normal));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Compile and link the shader program.
    // We use the Nvidia FW for creating the program; it's not difficult to do manually,
    // but takes about half a page of OpenGL boilerplate code.
    // This shader program will be used to draw everything except the user interface.
    // It consists of one vertex shader and one fragment shader.
    auto shader_program = new ShaderProgram(
        "#version 330\n"
        FW_GL_SHADER_SOURCE(
            layout(location = 0) in vec4 aPosition;
            layout(location = 1) in vec3 aNormal;

            out vec4 vColor;

            uniform mat4 uModelToWorld;
            uniform mat4 uWorldToClip;
            uniform float uShading;

            const vec3 distinctColors[6] = vec3[6](
                vec3(0, 0, 1), vec3(0, 1, 0), vec3(0, 1, 1),
                vec3(1, 0, 0), vec3(1, 0, 1), vec3(1, 1, 0));
            const vec3 directionToLight = normalize(vec3(0.5, 0.5, -0.6));

            void main()
            {
                // EXTRA: oops, someone forgot to transform normals here...
                float clampedCosine = clamp(dot(aNormal, directionToLight), 0.0, 1.0);
                vec3 litColor = vec3(clampedCosine);
                vec3 generatedColor = distinctColors[gl_VertexID % 6];
                // gl_Position is a built-in output variable that marks the final position
                // of the vertex in clip space. Vertex shaders must write in it.
                gl_Position = uWorldToClip * uModelToWorld * aPosition;
                vColor = vec4(mix(generatedColor, litColor, uShading), 1);
            }
        ),
        "#version 330\n"
        FW_GL_SHADER_SOURCE(
            in vec4 vColor;
            out vec4 fColor;
            void main()
            {
                fColor = vColor;
            }
        ));

    // Get the IDs of the shader program and its uniform input locations from OpenGL.
    gl_.shader_program = shader_program->getHandle();
    gl_.world_to_clip_uniform = glGetUniformLocation(gl_.shader_program, "uWorldToClip");
    gl_.model_to_world_uniform = glGetUniformLocation(gl_.shader_program, "uModelToWorld");
    gl_.shading_toggle_uniform = glGetUniformLocation(gl_.shader_program, "uShading");

    uploadGeometryToGPU(loadExampleModel());
}

//------------------------------------------------------------------------

vector<App::Vertex> App::loadObjFileModel(string filename)
{
    vector<Vector3f> positions, normals;
    vector<array<unsigned, 6>> faces;

    // Open input file stream for reading.
    ifstream input(filename, ios::in);

    // Read the file line by line.
    string line; 
    while (getline(input, line)) {
        // Replace any '/' characters with spaces ' ' so that all of the
        // values we wish to read are separated with whitespace.
        for (auto& c : line)
            if (c == '/')
                c = ' ';

        // Temporary objects to read data into.
        array<unsigned, 6>  f; // Face index array
        Vector3f               v;
        string              s;

        // Create a stream from the string to pick out one value at a time.
        istringstream        iss(line);

        // Read the first token from the line into string 's'.
        // It identifies the type of object (vertex or normal or ...)
        iss >> s;

        if (s == "v") { // vertex position
            // YOUR CODE HERE (R4)
            // Read the three vertex coordinates (x, y, z) into 'v'.
            // Store a copy of 'v' in 'positions'.
            // See std::vector documentation for push_back.
 
            iss >> v.x();
            iss >> v.y();
            iss >> v.z();
            positions.push_back(v);
        }
        else if (s == "vn") { // normal
            // YOUR CODE HERE (R4)
            // Similar to above.
            iss >> v.x();
            iss >> v.y();
            iss >> v.z();
            normals.push_back(v);
        }
        else if (s == "f") { // face
            // YOUR CODE HERE (R4)
            // Read the indices representing a face and store it in 'faces'.
            // The data in the file is in the format
            // f v1/vt1/vn1 v2/vt2/vn2 ...
            // where vi = vertex index, vti = texture index, vni = normal index.
            //
            // Remember we already replaced the '/' characters with whitespaces.
            //
            // Since we are not using textures in this exercise, you can ignore
            // the texture indices by reading them into a temporary variable.

            unsigned sink; // Temporary variable for reading the unused texture indices.

            // Note that in C++ we index things starting from 0, but face indices in OBJ format start from 1.
            // If you don't adjust for that, you'll index past the range of your vectors and get a crash.
                
            // It might be a good idea to print the indices to see that they were read correctly.
            iss >> f[0];
            iss >> sink;
            iss >> f[1];
           
            iss >> f[2];
            iss >> sink;
            iss >> f[3];
            
            iss >> f[4];
            iss >> sink;
            iss >> f[5];
            
            f[0] -= 1;
            f[1] -= 1;
            f[2] -= 1;
            f[3] -= 1;
            f[4] -= 1;
            f[5] -= 1;
            
            faces.push_back(f);
            //cout << f[0] << " " << f[1] << " " << f[2] << " " << f[3] << " " << f[4] << " " << f[5] << endl;
        }
    }
    //common_ctrl_.message(("Loaded mesh from " + filename).c_str());
    return unpackIndexedData(positions, normals, faces);
}

//------------------------------------------------------------------------
// Font management stuff

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

    // ScaleAllSizes affects too manty things, e.g. paddings and spacings
    //ImGui::GetStyle().ScaleAllSizes(ui_scale_);

    // io.FontGlobalScale just scales previously generated textures
    loadFont("roboto_mono.ttf", 14.0f * ui_scale_);
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
