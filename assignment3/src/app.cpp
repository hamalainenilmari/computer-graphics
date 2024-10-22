#include "app.h"

#include <fmt/core.h>

#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <numeric>
#include "im3d.h"
#include "im3d_math.h"

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
    if (!filesystem::is_directory(cwd / "assets")) {
        cout << fmt::format("Current working directory \"{}\" does not contain an \"assets\" folder.\n", cwd.string())
            << "Make sure the executable gets run relative to the project root.\n";
        return;
    }

    static_assert(is_standard_layout<VertexPNC>::value, "Vertex must be standard layout to use offsetof");
    static_assert(is_standard_layout<WeightedVertex>::value, "WeightedVertex must be standard layout to use offsetof");

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
    window_ = glfwCreateWindow(1920, 1080, "CS-C3100 Computer Graphics, Assignment 3", NULL, NULL);
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

    if (!Im3d_Init())
        fail("Error initializing Im3d!");

    {
        static const Vector3f distinct_colors[6] = {
            Vector3f(0, 0, 1), Vector3f(0, 1, 0), Vector3f(0, 1, 1),
            Vector3f(1, 0, 0), Vector3f(1, 0, 1), Vector3f(1, 1, 0) };
        for (auto i = 0u; i < 100; ++i)
            joint_colors_.push_back(distinct_colors[i % 6]);
    }

    int current_model = 0;
    // If you want to include other BVH files, you can insert their paths here.
    // REMEMBER to include them in your assets/ directory in your submission if
    // you plan to show them to us!
    static array<const char*, 2> model_list = { "assets/characters/mocapguy.bvh",
                                                "assets/characters/lafan1/dance1_subject1.bvh" };

    // generate vertex buffer objects, load shaders, etc.
    initRendering();

    // load default skeleton & animation
    loadCharacter(model_list[current_model]);

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

        if (ImGui::Combo("Model", &current_model, model_list.data(), model_list.size()))
            loadCharacter(model_list[current_model]);

        ImGui::Text("Draw mode");
        ImGui::SameLine();
        if (ImGui::RadioButton("Skeleton (1)", draw_mode_ == DrawMode_Skeleton))
            draw_mode_ = DrawMode_Skeleton;
        ImGui::SameLine();
        if (ImGui::RadioButton("CPU SSD (2)", draw_mode_ == DrawMode_SSD_CPU) && weighted_vertices_.size() > 0)
            draw_mode_ = DrawMode_SSD_CPU;
        ImGui::SameLine();
        if (ImGui::RadioButton("GPU SSD (3)", draw_mode_ == DrawMode_SSD_GPU) && weighted_vertices_.size() > 0)
            draw_mode_ = DrawMode_SSD_GPU;

        if (draw_mode_ == DrawMode_Skeleton)
            ImGui::Checkbox("Draw joint frames", &draw_joint_frames_);
        else
            ImGui::Checkbox("Shading", &shading_toggle_);

        ImGui::Checkbox("Animate (SPACE)", &animation_mode_);

        // check if we need to update the joint angles & update animation time if running
        if (ImGui::SliderFloat("Animation time",
            &animation_current_time_,
            0.0f,
            (skel_.getNumAnimationFrames()-1) * skel_.getAnimationFrameTime())
            || animation_mode_)
        {
            if (animation_mode_)
            {
                animation_current_time_ += io_->DeltaTime * powf(2.0f, animation_speed_exponent_);
                animation_current_time_ = fmod(animation_current_time_, (skel_.getNumAnimationFrames() - 1) * skel_.getAnimationFrameTime());
            }

            float animFrame = animation_current_time_ / skel_.getAnimationFrameTime();
            skel_.setAnimationFrame(animFrame);
        }

        {
            string pbs_format;
            if (animation_speed_exponent_ >= 0)
                pbs_format = fmt::format("{}x", int(pow(2.0, animation_speed_exponent_)));
            else
                pbs_format = fmt::format("1/{}x", int(pow(2.0, -animation_speed_exponent_)));

            ImGui::SliderInt("Playback speed", &animation_speed_exponent_, -4, 4, pbs_format.c_str());
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

void App::initRendering()
{
    // Create vertex attribute objects and buffers for vertex data.
    glAssert(glGenVertexArrays(1, &gl_.simple_vao));
    glAssert(glGenVertexArrays(1, &gl_.ssd_vao));
    glAssert(glGenBuffers(1, &gl_.simple_vertex_buffer));
    glAssert(glGenBuffers(1, &gl_.ssd_vertex_buffer));
    
    // Set up vertex attribute object for doing SSD on the CPU. The data will be loaded and re-loaded later, on each frame.
    glBindVertexArray(gl_.simple_vao);
    glBindBuffer(GL_ARRAY_BUFFER, gl_.simple_vertex_buffer);
    glEnableVertexAttribArray(ATTRIB_POSITION);
    glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPNC), (GLvoid*) 0);
    glEnableVertexAttribArray(ATTRIB_NORMAL);
    glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPNC), (GLvoid*) offsetof(VertexPNC, normal));
    glEnableVertexAttribArray(ATTRIB_COLOR);
    glVertexAttribPointer(ATTRIB_COLOR, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPNC), (GLvoid*) offsetof(VertexPNC, color));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // Set up vertex attribute object for doing SSD on the GPU.
    // Data will be uploaded in loadCharacter().
    glBindVertexArray(gl_.ssd_vao);
    glBindBuffer(GL_ARRAY_BUFFER, gl_.ssd_vertex_buffer);
    glEnableVertexAttribArray(ATTRIB_POSITION);
    glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(WeightedVertex), (GLvoid*) 0);
    glEnableVertexAttribArray(ATTRIB_NORMAL);
    glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(WeightedVertex), (GLvoid*) offsetof(WeightedVertex, normal));
    glEnableVertexAttribArray(ATTRIB_COLOR);
    glVertexAttribPointer(ATTRIB_COLOR, 3, GL_FLOAT, GL_FALSE, sizeof(WeightedVertex), (GLvoid*) offsetof(WeightedVertex, color));
    glEnableVertexAttribArray(ATTRIB_JOINTS1);
    glVertexAttribIPointer(ATTRIB_JOINTS1, 4, GL_INT, sizeof(WeightedVertex), (GLvoid*) offsetof(WeightedVertex, joints[0]));
    glEnableVertexAttribArray(ATTRIB_JOINTS2);
    glVertexAttribIPointer(ATTRIB_JOINTS2, 4, GL_INT, sizeof(WeightedVertex), (GLvoid*) offsetof(WeightedVertex, joints[4]));
    glEnableVertexAttribArray(ATTRIB_WEIGHTS1);
    glVertexAttribPointer(ATTRIB_WEIGHTS1, 4, GL_FLOAT, GL_FALSE, sizeof(WeightedVertex), (GLvoid*) offsetof(WeightedVertex, weights[0]));
    glEnableVertexAttribArray(ATTRIB_WEIGHTS2);
    glVertexAttribPointer(ATTRIB_WEIGHTS2, 4, GL_FLOAT, GL_FALSE, sizeof(WeightedVertex), (GLvoid*) offsetof(WeightedVertex, weights[4]));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Compile and link the shader programs.

    auto simple_program = new ShaderProgram(
        "#version 330\n"
        "#extension GL_ARB_separate_shader_objects : enable\n"
        FW_GL_SHADER_SOURCE(
        layout(location = 0) in vec4 aPosition;
        layout(location = 1) in vec3 aNormal;
        layout(location = 2) in vec3 aColor;

        out vec4 vColor;

        uniform mat4 uWorldToClip;
        uniform float uShadingMix;

        const vec3 directionToLight = normalize(vec3(0.5, 0.5, 0.6));

        void main()
        {
            float clampedCosine = clamp(dot(aNormal, directionToLight), 0.0, 1.0);
            vec3 litColor = vec3(clampedCosine);
            vColor = vec4(mix(aColor.xyz, litColor, uShadingMix), 1);
            gl_Position = uWorldToClip * aPosition;
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
    // YOUR CODE HERE (EXTRA):
    // Perform the skinning math in the vertex shader, like the example binary
    // does. This is the exact same thing you already did in R4 & R5, only
    // translated from C++ to GLSL. Remember to handle normals as well.
    auto ssd_program = new ShaderProgram(
        "#version 330\n"
        "#extension GL_ARB_separate_shader_objects : enable\n"
        FW_GL_SHADER_SOURCE(
        layout(location = 0) in vec4 aPosition;
        layout(location = 1) in vec3 aNormal;
        layout(location = 2) in vec4 aColor;
        layout(location = 3) in ivec4 aJoints1;
        layout(location = 4) in ivec4 aJoints2;
        layout(location = 5) in vec4 aWeights1;
        layout(location = 6) in vec4 aWeights2;

        const vec3 directionToLight = normalize(vec3(0.5, 0.5, 0.6));
        uniform mat4 uWorldToClip;
        uniform float uShadingMix;

        out vec4 vColor;		

        const int numJoints = 100;
        uniform mat4 uJoints[numJoints];
    
        void main()
        {
            float clampedCosine = clamp(dot(aNormal, directionToLight), 0.0, 1.0);
            // this next line is just to force a reference to the uJoints uniform;
            // without it the compiler optimizes the reference away and
            // the rendering code doesn't work.
            clampedCosine += 1e-8 * uJoints[0][0][0];
            vec3 litColor = vec3(clampedCosine);
            vColor = vec4(mix(aColor.xyz, litColor, uShadingMix), 1);
            gl_Position = uWorldToClip * aPosition;
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

    // Get the IDs of the shader programs and their uniform input locations from OpenGL.
    gl_.ssd_shader = ssd_program->getHandle();
    gl_.ssd_transforms_uniform = glGetUniformLocation(gl_.ssd_shader, "uJoints");
    gl_.ssd_world_to_clip_uniform = glGetUniformLocation(gl_.ssd_shader, "uWorldToClip");
    gl_.ssd_shading_mix_uniform = glGetUniformLocation(gl_.ssd_shader, "uShadingMix");
    gl_.simple_shader = simple_program->getHandle();
    gl_.simple_world_to_clip_uniform = glGetUniformLocation(gl_.simple_shader, "uWorldToClip");
    gl_.simple_shading_mix_uniform = glGetUniformLocation(gl_.simple_shader, "uShadingMix");
}

void App::render(int window_width, int window_height, vector<string>& vecStatusMessages)
{
    // Clear screen
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // consider clockwise vertex ordering to be the "front"
    glFrontFace(GL_CW);

    // tell OpenGL the size of the surface we're drawing into
    glViewport(0, 0, window_width, window_height);

    // Set up transform matrices.
    // They will be fed to the shader program via uniform variables.

    // World space -> clip space transform: simple projection and camera.

    // Our camera orbits around (0, camera_height_, 0) at camera_distance_.
    Matrix4f C = Matrix4f::Identity();
    Matrix3f rot = Matrix3f(AngleAxis<float>(-camera_rotation_, Vector3f(0, 1, 0)));
    C.block(0, 0, 3, 3) = rot;
    C.block(0, 3, 3, 1) = Vector3f(0, 0, camera_distance_);
    Matrix4f T = Matrix4f::Identity();
    T.block(0, 3, 3, 1) = Vector3f(0.0f, -camera_height_, 0.0f);
    float xangle = atan2f(camera_height_ - 1.0f, camera_distance_);
    Matrix4f xrot;
    xrot << Matrix3f(AngleAxis<float>(-xangle, Vector3f(1, 0, 0))), Vector3f::Zero(), RowVector4f(0, 0, 0, 1);
    C = xrot * C * T;

    static const float fNear = 0.1f, fFar = 15.0f;
    float fAspect = float(window_width) / window_height;
    Matrix4f P{
        { 1.0f,    0.0f,                            0.0f,                               0.0f },
        { 0.0f, fAspect,                            0.0f,                               0.0f },
        { 0.0f,    0.0f, (fFar + fNear) / (fFar - fNear), -2 * fFar * fNear / (fFar - fNear) },
        { 0.0f,    0.0f,                            1.0f,                               0.0f }
    };
    Matrix4f world_to_clip = P * C;

    double mousex, mousey;
    glfwGetCursorPos(window_, &mousex, &mousey);
    Im3d_NewFrame(window_, window_width, window_height, C, P, 0.01f, mousex, mousey);

    if (draw_mode_ == DrawMode_Skeleton)
    {
        // Draw the skeleton as a set of joint positions, connecting lines,
        // and local coordinate systems at each joint using Im3d's immediate
        // mode API for drawing lines, points, and other primitives.
        glUseProgram(0);
        renderSkeleton(vecStatusMessages);
    }
    else if (draw_mode_ == DrawMode_SSD_CPU)
    {
        static vector<VertexPNC> skinned_vertices;
        computeSSD(weighted_vertices_, skinned_vertices);

        glBindBuffer(GL_ARRAY_BUFFER, gl_.simple_vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(VertexPNC) * skinned_vertices.size(), skinned_vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glUseProgram(gl_.simple_shader);
        glUniformMatrix4fv(gl_.simple_world_to_clip_uniform, 1, GL_FALSE, world_to_clip.data());
        glUniform1f(gl_.simple_shading_mix_uniform, shading_toggle_ ? 1.0f : 0.0f);
        glBindVertexArray(gl_.simple_vao);
        glDrawArrays(GL_TRIANGLES, 0, (int)skinned_vertices.size());
        glBindVertexArray(0);
        glUseProgram(0);
    }
    else if (draw_mode_ == DrawMode_SSD_GPU)
    {
        static vector<Matrix4f> ssd_transforms;
        skel_.getSSDTransforms(ssd_transforms);

        // When working on GPU skinning extra, REMOVE THIS and just pass
        // in world_to_clip.data() in the 1st glUniformMatrix4fv call!
        Matrix4f gpu_ssd_object_to_clip = world_to_clip * skel_.getObjectToWorldTransform();

        glUseProgram(gl_.ssd_shader);
        glUniformMatrix4fv(gl_.ssd_world_to_clip_uniform, 1, GL_FALSE, gpu_ssd_object_to_clip.data());
        glUniform1f(gl_.ssd_shading_mix_uniform, shading_toggle_ ? 1.0f : 0.0f);
        glUniformMatrix4fv(gl_.ssd_transforms_uniform, GLsizei(ssd_transforms.size()), GL_FALSE, (GLfloat*)ssd_transforms.data());
        
        glBindVertexArray(gl_.ssd_vao);
        glDrawArrays(GL_TRIANGLES, 0, (int)weighted_vertices_.size());
        glBindVertexArray(0);
        glUseProgram(0);
    }
    
    // Show status messages.
    vecStatusMessages.push_back("Use Q/W to change selected bone\n    Arrow keys and PgUp/Dn to rotate selected bone\n    R to reset current bone euler_angles\n    Left mouse button + drag rotates\n    Right mouse + drag move fwd/back\n    Middle mouse + drag move up/down");
    Vector3f joint_rot = skel_.getJointRotation(selected_joint_);
    Vector3f joint_pos = skel_.getToWorldTransform(selected_joint_).block(0, 3, 3, 1);

    vecStatusMessages.push_back(fmt::format("Joint \"{}\" selected\n  euler_angles {:.2f} {:.2f} {:.2f}\n  world pos {:.2f} {:.2f} {:.2f}",
                        skel_.getJointName(selected_joint_).c_str(),
                        joint_rot(0), joint_rot(1), joint_rot(2),
                        joint_pos(0), joint_pos(1), joint_pos(2)));

    // Draw grid on y = 0 plane
    Im3d::BeginLines();
    Im3d::SetSize(2.0f);
    Im3d::SetColor(0.6f, 0.6f, 0.6f);
    const int grid_size = 17;
    const float grid_cell = 0.25f;
    const float grid_y = 0.0f;
    for ( int i = -grid_size; i <= grid_size; ++i )
        for (int j = -grid_size; j <= grid_size; ++j)
        {
            // x = const. line
            Im3d::Vertex(i * grid_cell, grid_y, -grid_size * grid_cell);
            Im3d::Vertex(i * grid_cell, grid_y,  grid_size * grid_cell);
            // z = const. line
            Im3d::Vertex(-grid_size * grid_cell, grid_y, j * grid_cell);
            Im3d::Vertex( grid_size * grid_cell, grid_y, j * grid_cell);
        }
    Im3d::End();

    // this draws the accumulated line and point buffers
    Im3d_EndFrame();
}

void App::renderSkeleton(vector<string>& vecStatusMessages)
{
    // Let's fetch the transforms you generated in Skeleton::updateToWorldTransforms().
    // You need them in R1 and R3.
    static vector<Matrix4f> joint_to_world_transforms;
    skel_.getToWorldTransforms(joint_to_world_transforms);

    // And loop through all the joints.
    for (auto i = 0u; i < joint_to_world_transforms.size(); ++i) {
        // YOUR CODE HERE (R1)
        // Use the transforms to obtain the position of the joint in world space.
        // This should be a one-liner.
        Vector3f joint_world_pos = Vector3f(joint_to_world_transforms[i](0, 3), joint_to_world_transforms[i](1, 3), joint_to_world_transforms[i](2, 3));
        Im3d::BeginPoints();
        if (i == (int)selected_joint_)
        {
            Im3d::SetSize(32.0f);
            Im3d::SetColor(1.0f, 0.2f, 0.2f);
        }
        else
        {
            Im3d::SetSize(16.0f);
            Im3d::SetColor(1.0f, 1.0f, 1.0f);
        }
        Im3d::Vertex(joint_world_pos);
        Im3d::End();

        Im3d::SetSize(4.0f);

        if (draw_joint_frames_)
        {
            // YOUR CODE HERE (R3)
            // Use the transforms to obtain the joint's orientation.
            // (If you understand transformation matrices correctly, you can directly
            // read the these vectors off of the matrices.)
            Vector3f right(Vector3f::Zero());
            Vector3f up(Vector3f::Zero());
            Vector3f ahead(Vector3f::Zero());
            // Then let's draw some lines to show the joint coordinate system.
            // Draw a small coloured line segment from the joint's world position towards
            // each of its local coordinate axes (the line length should be determined by "scale").
            // The colors for each axis are already set for you below.
            float scale = i == selected_joint_ ? 10.0f : 2.5f;	// length for the coordinate system axes.
            Im3d::BeginLines();

            // draw the x axis... ("right")
            Im3d::SetColor(1.0, 0.0f, 0.0f);
            //Im3d::Vertex(...);
            //Im3d::Vertex(...);

            // ..and the y axis.. ("up")
            Im3d::SetColor(0, 1, 0); // green
            //Im3d::Vertex(...);
            //Im3d::Vertex(...);

            // ..and the z axis ("ahead").
            Im3d::SetColor(0, 0, 1); // blue
            //Im3d::Vertex(...);
            //Im3d::Vertex(...);
            Im3d::End();  // done with frame lines
        }

        Im3d::SetColor(1, 1, 1); // white
        Im3d::BeginLines();
        // Finally, draw a line segment from the world position of this joint to the world
        // position of the parent joint. Make sure to account for the root node 0 correctly.
            
        // ...
        Im3d::End(); // we're done drawing lines	
    }
}

void App::computeSSD(const vector<WeightedVertex>& source_vertices, vector<VertexPNC>& dest)
{
    static vector<Matrix4f> ssd_transforms;
    skel_.getSSDTransforms(ssd_transforms);
    dest.clear();
    dest.reserve(source_vertices.size());
    VertexPNC v;
    for (const auto& sv : source_vertices)
    {
        // YOUR CODE HERE (R4 & R5)
        // Compute the skinned position for each vertex and push it to the "dest" vector.
        // This starter code just applies the skeleton's object-to-world transformation to
        // the source vertex, so the result is not animated.
        // Replace these lines with the loop that applies the bone transforms and weights.
        // For R5, transform the normals as well.
        // NOTE that the object-to-world transformation is already baked in to the
        // joint-to-world transformations, so your solution does not have to explicitly
        // do it; you do not need the object_to_world matrix in the solution. It is
        // included here to make the unskinned bind-pose mesh reasonably sized and positioned.

        Vector4f p4;
        p4 << sv.position, 1.0f;
        v.position = (skel_.getObjectToWorldTransform() * p4)({ 0, 1, 2 }); // fancy way of getting 1st three components from Vector4f
        v.normal = sv.normal;
        v.color = sv.color;

        dest.push_back(v);
    }
}

vector<WeightedVertex> App::loadAnimatedMesh(string namefile, string mesh_file, string attachment_file)
{
    vector<WeightedVertex> vertices;
    vector<array<int, WEIGHTS_PER_VERTEX>> indices;
    vector<array<float, WEIGHTS_PER_VERTEX>> weights;
    vector<Vector3f> colors;
    
    // Load name to index conversion map
    vector<string> names;
    ifstream name_input(namefile, ios::in);
    string name;
    while (name_input.good())
    {
        name_input >> name;
        names.push_back(name);
    }

    // Load vertex weights
    ifstream attachment_input(attachment_file, ios::in);
    string line;
    while (getline(attachment_input, line)) {
        auto temp_i = array<int, WEIGHTS_PER_VERTEX>();
        auto temp_w = array<float, WEIGHTS_PER_VERTEX>();
        stringstream ss(line);
        int sink;
        ss >> sink;
        auto n_weights = 0u;
        while(ss.good()) {
            string word;
            ss >> word;
            int i = stoi(word.substr(0, word.find_first_of('.')));
            float weight;
            ss >> weight;
            if (weight != 0) {
                temp_w[n_weights] = weight;
                temp_i[n_weights] = skel_.getJointIndex(names[names.size()-i-2]);
                ++n_weights;
            }
        }
        assert(n_weights <= WEIGHTS_PER_VERTEX);
        weights.push_back(temp_w);
        indices.push_back(temp_i);
        Vector3f color = Vector3f::Zero();
        for (auto i = 0u; i < WEIGHTS_PER_VERTEX; ++i)
            color += joint_colors_[temp_i[i]] * temp_w[i];
        colors.push_back(color);
    }

    // Load vertices
    vector<Vector3f> positions;
    ifstream mesh_input(mesh_file, ios::in);
    while (getline(mesh_input, line)) {
        stringstream ss(line);
        string s;
        ss >> s;
        if (s == "v") {
            Vector3f pos;
            ss >> pos[0] >> pos[1] >> pos[2];
            positions.push_back(pos);
        }
        else if (s == "f") {
            array<int, 3> f;

            string word;
            for (int i = 0; i < 3; ++i)
            {
                ss >> word;
                word = word.substr(0, word.find_first_of('/'));
                f[i] = stoi(word) - 1;
            }

            WeightedVertex v;
            v.normal = (positions[f[1]] - positions[f[0]]).cross(positions[f[2]] - positions[f[0]]).normalized();
            for (auto i : f)
            {
                v.position = positions[i];
                v.color = colors[i];
                memcpy(v.joints, indices[i].data(), sizeof(v.joints));
                memcpy(v.weights, weights[i].data(), sizeof(v.weights));
                vertices.push_back(v);
            }

            // Read n-gons
            while (ss.good())
            {
                f[1] = f[2];
                ss >> word;
                word = word.substr(0, word.find_first_of('/'));
                f[2] = stoi(word) - 1;

                v.normal = (positions[f[1]] - positions[f[0]]).cross(positions[f[2]] - positions[f[0]]).normalized();
                for (auto i : f)
                {
                    v.position = positions[i];
                    v.color = colors[i];
                    memcpy(v.joints, indices[i].data(), sizeof(v.joints));
                    memcpy(v.weights, weights[i].data(), sizeof(v.weights));
                    vertices.push_back(v);
                }
            }
        }
    }

    int idx = 0;
    for (const auto& v : vertices)
    {
        float sum_weights = accumulate(begin(v.weights), end(v.weights), 0.0f);
        (void)sum_weights; // silence warning on release build
        assert(0.99 < sum_weights && sum_weights < 1.01 && "weights do not sum up to 1");
        for (auto i = 0u; i < WEIGHTS_PER_VERTEX; ++i) {
            assert(0 <= v.joints[i] && "invalid index");
        }
        idx++;
    }
    return vertices;
}

void App::loadCharacter(const string& filename)
{
    if (!filesystem::exists(filename))
        fail(fmt::format("Tried to load '{}', doesn't exist!", filename));

    auto end = filename.find_last_of('.');
    string prefix = (end > 0) ? filename.substr(0, end) : filename;
    string skel_file = prefix + ".bvh";
    string mesh_file = prefix + ".bvhobj";
    string weight_file = prefix + ".weights";
    string name_file = prefix + ".names";

    bool comesWithSkin = filesystem::exists(mesh_file) &&
                         filesystem::exists(weight_file) &&
                         filesystem::exists(name_file);

    cout << "skeleton:   " << skel_file << endl;

    weighted_vertices_.clear();

    skel_.loadBVH(skel_file, comesWithSkin);

    if (comesWithSkin)
    {
        cout << "mesh:       " << mesh_file << endl;
        cout << "weight:     " << weight_file << endl;
        cout << "name:       " << name_file << endl;
        weighted_vertices_ = loadAnimatedMesh(name_file, mesh_file, weight_file);

        // upload vertex data to GPU
        glBindVertexArray(gl_.ssd_vao);
        glBindBuffer(GL_ARRAY_BUFFER, gl_.ssd_vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(WeightedVertex) * weighted_vertices_.size(), weighted_vertices_.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    animation_current_time_ = 0.0f;
    draw_mode_ = DrawMode_Skeleton;
}

void App::handleDrop(GLFWwindow* window, int count, const char** paths) {}
void App::handleKeypress(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        static const float rot_incr = 1.5;

        if (key == GLFW_KEY_HOME)
            camera_rotation_ -= 0.05 * EIGEN_PI;
        else if (key == GLFW_KEY_END)
            camera_rotation_ += 0.05 * EIGEN_PI;
        else if (key == GLFW_KEY_UP)
            skel_.incrJointRotation(selected_joint_, Vector3f(rot_incr, 0, 0));
        else if (key == GLFW_KEY_DOWN)
            skel_.incrJointRotation(selected_joint_, Vector3f(-rot_incr, 0, 0));
        else if (key == GLFW_KEY_LEFT)
            skel_.incrJointRotation(selected_joint_, Vector3f(0, rot_incr, 0));
        else if (key == GLFW_KEY_RIGHT)
            skel_.incrJointRotation(selected_joint_, Vector3f(0, -rot_incr, 0));
        else if (key == GLFW_KEY_PAGE_UP)
            skel_.incrJointRotation(selected_joint_, Vector3f(0, 0, rot_incr));
        else if (key == GLFW_KEY_PAGE_DOWN)
            skel_.incrJointRotation(selected_joint_, Vector3f(0, 0, -rot_incr));
        else if (key == GLFW_KEY_R)
        {
            skel_.setJointRotation(selected_joint_, Vector3f::Zero());
            skel_.updateToWorldTransforms();
        }
        else if (key == GLFW_KEY_Q)
            selected_joint_ = selected_joint_ == 0 ? 0 : selected_joint_ - 1;
        else if (key == GLFW_KEY_W)
            selected_joint_ = clamp(selected_joint_ + 1, 0u, (unsigned)skel_.getNumJoints() - 1u);
        else if (key == GLFW_KEY_SPACE)
            animation_mode_ = !animation_mode_;
        else if (key == GLFW_KEY_1)
            draw_mode_ = DrawMode_Skeleton;
        else if (key == GLFW_KEY_2 && weighted_vertices_.size() > 0)
            draw_mode_ = DrawMode_SSD_CPU;
        else if (key == GLFW_KEY_3 && weighted_vertices_.size() > 0)
            draw_mode_ = DrawMode_SSD_GPU;
        else if (key == GLFW_KEY_O)
            decreaseUIScale();
        else if (key == GLFW_KEY_P)
            increaseUIScale();
    }
}

void App::handleMouseButton(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
    }
    else if (action == GLFW_RELEASE)
    {
    }
}

void App::handleMouseMovement(GLFWwindow* window, double xpos, double ypos)
{
    // if the user is interacting with an Im3d gizmo, don't do anything else
    if (Im3d::GetContext().m_activeId != 0)
        return;

    static double old_x_pos = 0.0, old_y_pos = 0.0;

    if ( glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS )
        camera_rotation_ += 0.01f * (xpos - old_x_pos);
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
        camera_distance_ -= 0.01f * (ypos - old_y_pos);
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
        camera_height_ += 0.01f * (ypos - old_y_pos);

    camera_distance_ = max(camera_distance_, 1.0f);

    old_x_pos = xpos;
    old_y_pos = ypos;
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
