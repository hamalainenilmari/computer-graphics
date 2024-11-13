#include "app.h"

#include <fmt/core.h>

#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <numeric>
#include <map>
#include "im3d.h"
#include "im3d_math.h"
#include "implot.h"

#include "integrators.h"

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


namespace {

    enum VertexShaderAttributeLocations
    {
        ATTRIB_POSITION = 0,
        ATTRIB_NORMAL = 1
    };

} // namespace

App::App(void)
    : pendulum_system_(4),
      cloth_system_(20, 20),
      sprinkler_system_(10)
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

    static_assert(is_standard_layout<Vertex>::value, "struct Vertex must be standard layout to use offsetof");

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
    window_ = glfwCreateWindow(1920, 1080, "CS-C3100 Computer Graphics, Assignment 4", NULL, NULL);
    if (!window_) {
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

    if (!Im3d_Init())
        fail("Error initializing Im3d!");

    initRendering();

    setUIScale(1.5f);

    vector<string> vecStatusMessages;

    static array<const char*, 4> integrator_list = { "EULER (F1)", "TRAPEZOID (F2)", "MIDPOINT (F3)", "RK4 (F4)" };
    static array<IntegratorType, 4> name2integrator = { EULER_INTEGRATOR, TRAPEZOID_INTEGRATOR, MIDPOINT_INTEGRATOR, RK4_INTEGRATOR };
    static map<IntegratorType, int> integrator2index;
    integrator2index[EULER_INTEGRATOR] = 0;
    integrator2index[TRAPEZOID_INTEGRATOR] = 1;
    integrator2index[MIDPOINT_INTEGRATOR] = 2;
    integrator2index[RK4_INTEGRATOR] = 3;

    static array<const char*, 5> system_list = { "Simple (1)", "Spring (2)", "Multi-pendulum (3)", "Cloth (4)", "Sprinkler (5)"};
    static array<ParticleSystemType, 5> name2system = { SIMPLE_SYSTEM, SPRING_SYSTEM, PENDULUM_SYSTEM, CLOTH_SYSTEM, SPRINKLER_SYSTEM};
    static map<ParticleSystemType, int> system2index;
    system2index[SIMPLE_SYSTEM] = 0;
    system2index[SPRING_SYSTEM] = 1;
    system2index[PENDULUM_SYSTEM] = 2;
    system2index[CLOTH_SYSTEM] = 3;
    system2index[SPRINKLER_SYSTEM] = 4;

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

        step();

        int width, height;
        glfwGetFramebufferSize(window_, &width, &height);
        render(width, height, vecStatusMessages);

        // Begin GUI window
        if (ImGui::Begin("Controls"))
        {
            int selected_system = system2index[ps_type_];
            if (ImGui::Combo("System", &selected_system, system_list.data(), system_list.size()))
            {
                ps_type_ = name2system[selected_system];
                switch (ps_type_)
                {
                case SIMPLE_SYSTEM:
                    ps_ = &simple_system_; break;
                case SPRING_SYSTEM:
                    ps_ = &spring_system_; break;
                case PENDULUM_SYSTEM:
                    ps_ = &pendulum_system_; break;
                case CLOTH_SYSTEM:
                    ps_ = &cloth_system_; break;
                case SPRINKLER_SYSTEM:
                    ps_ = &sprinkler_system_; break;
                default:
                    assert(false && "invalid system type");
                }
                ps_->reset();
            }

            // add the current particle system's UI elements
            ps_->imgui_interface();

            if (ps_type_ == CLOTH_SYSTEM)
                ImGui::Checkbox("Shading", &shading_toggle_);

            int selected_integrator = integrator2index[integrator_];
            if (ImGui::Combo("Integrator", &selected_integrator, integrator_list.data(), integrator_list.size()))
                integrator_ = name2integrator[selected_integrator];

            static int stepsize_log10 = int(log10(double(step_)));
            if (ImGui::SliderInt("Step size (dt)", &stepsize_log10, -10, 0, fmt::format("{:.1e}", pow(10.0, stepsize_log10)).c_str()))
                step_ = float(pow(10.0, stepsize_log10));

            static int steps_per_update_log2 = int(log2(double(steps_per_update_)));
            if (ImGui::SliderInt("Steps per update", &steps_per_update_log2, 0, 8, fmt::format("{}", 1 << steps_per_update_log2).c_str()))
                steps_per_update_ = 1 << steps_per_update_log2;


            if (ImGui::Button("Reset system (R)"))
                ps_->reset();

            // This is for interactive visualization of the dimensions in the phase space.
            // Experimental and not fully thought through.
            if (ImGui::TreeNodeEx("Phase portrait"))
            {
                size_t n = ps_->state().size();

                // make sure the plot dimensions are legal
                static int dims[2] = { 0, 1 };
                if (dims[0] > n || dims[1] > n)
                {
                    dims[0] = 0;
                    dims[1] = 1;
                }

                // allow the user to change the dimensions
                ImGui::SliderInt2("Dimensions", dims, 0, n);

                if (ImPlot::BeginPlot("Phase portrait", ImVec2(-1,0), ImPlotFlags_Equal))
                {
                    const int num_states = 4096;
                    static vector<VectorXf> past_states;
                    static size_t current_entry = 0;
                    if (past_states.size() == 0 || past_states[0].size() != ps_->state().size())
                        past_states.assign(num_states, VectorXf(ps_->state()));
                    past_states[current_entry] = ps_->state();
                    auto newest_entry = current_entry;
                    current_entry = (current_entry + 1) % past_states.size();

                    static array<float, num_states> x, y;
                    for (size_t i = 0; i < num_states; ++i)
                    {
                        x[i] = past_states[i](dims[0]);
                        y[i] = past_states[i](dims[1]);
                    }
                    ImPlot::SetupAxes(ps_->dimension_name(dims[0]).c_str(), ps_->dimension_name(dims[1]).c_str());
                    if (current_entry > newest_entry)
                        ImPlot::PlotLine("Orbit", &x[current_entry], &y[current_entry], num_states - current_entry);
                    ImPlot::PlotLine("Orbit", x.data(), y.data(), current_entry);
                    ImPlot::PlotScatter("Current", &x[newest_entry], &y[newest_entry], 1);
                    ImPlot::EndPlot();
                }
                vecStatusMessages.push_back("Use mouse drag & wheel to navigate plot");
                ImGui::TreePop();
            }

            // Draw the status messages in the current list
            // Note that you can add them from inside render(...)
            vecStatusMessages.push_back(fmt::format("Application average {:.3f} ms/frame ({:.1f} FPS)", 1000.0f / io_->Framerate, io_->Framerate));
            vecStatusMessages.push_back("Home/End and mouse drag rotate camera");
            for (const string& msg : vecStatusMessages)
                ImGui::Text("%s", msg.c_str());
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

void App::step()
{
    for (int i = 0; i < steps_per_update_; ++i) {
        // sprinkler only uses the simple integrator for the sake of simplicity 
        if (SprinklerSystem* sprinkler = dynamic_cast<SprinklerSystem*>(ps_)) {
            sprinkler->update(step_);
            sprinkler->emit();
        }
        else {
            switch (integrator_)
            {
            case EULER_INTEGRATOR:
                stepSystem(*ps_, eulerStep, step_); break;
            case TRAPEZOID_INTEGRATOR:
                stepSystem(*ps_, trapezoidStep, step_); break;
            case MIDPOINT_INTEGRATOR:
                stepSystem(*ps_, midpointStep, step_); break;
            case RK4_INTEGRATOR:
                stepSystem(*ps_, rk4Step, step_); break;
            default:
                assert(false && " invalid integrator type");
            }
        }
    }
}

void App::initRendering()
{
    glGenBuffers(1, &gl_.vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, gl_.vertex_buffer);

    // Mesh rendering setup
    glGenVertexArrays(1, &gl_.mesh_vao);
    glBindVertexArray(gl_.mesh_vao);
    glEnableVertexAttribArray(ATTRIB_POSITION);
    glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
    glEnableVertexAttribArray(ATTRIB_NORMAL);
    glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, normal));

    // Point and line rendering setup
    glGenVertexArrays(1, &gl_.point_vao);
    glBindVertexArray(gl_.point_vao);
    glEnableVertexAttribArray(ATTRIB_POSITION);
    glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3f), (GLvoid*)0);
    glBindVertexArray(0);

    auto shader_program = new ShaderProgram(
        "#version 330\n"
        FW_GL_SHADER_SOURCE(
            layout(location = 0) in vec4 aPosition;
    layout(location = 1) in vec3 aNormal;

    out vec4 vColor;

    uniform mat4 uWorldToClip;

    const vec3 directionToLight = normalize(vec3(0.5, 0.3, 0.6));

    void main()
    {
        float clampedCosine = clamp(dot(aNormal, directionToLight), 0.0, 1.0);
        vec3 litColor = vec3(clampedCosine);
        gl_Position = uWorldToClip * aPosition;
        vColor = vec4(litColor, 1);
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
}

void App::render(int window_width, int window_height, vector<string>& vecStatusMessages)
{
    // Clear screen.
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Tell OpenGL the size of the draw surface.
    glViewport(0, 0, window_width, window_height);

    // Enable depth testing.
    glEnable(GL_DEPTH_TEST);

    // Our camera orbits around origin at a fixed distance.
    static const float cameraDistance = 2.1f;
    Matrix4f C = Matrix4f::Identity();
    Matrix3f rot = Matrix3f(AngleAxis<float>(-camera_rotation_angle_, Vector3f(0, 1, 0)));
    C.block(0, 0, 3, 3) = rot;
    C.col(3) = Vector4f(0, 0, cameraDistance, 1);

    // Simple perspective.
    static const float fNear = 0.1f, fFar = 4.0f;
    float fAspect = float(window_width) / window_height;
    Matrix4f P{
        { 1.0f,    0.0f,                            0.0f,                               0.0f },
        { 0.0f, fAspect,                            0.0f,                               0.0f },
        { 0.0f,    0.0f, (fFar + fNear) / (fFar - fNear), -2 * fFar * fNear / (fFar - fNear) },
        { 0.0f,    0.0f,                            1.0f,                               0.0f }
    };

    Matrix4f worldToClip = P * C;

    double mousex, mousey;
    glfwGetCursorPos(window_, &mousex, &mousey);
    Im3d_NewFrame(window_, window_width, window_height, C, P, 0.01f, mousex, mousey);

    if (ps_type_ != CLOTH_SYSTEM || !shading_toggle_)
        ps_->render(ps_->state());

    if (ps_type_ == CLOTH_SYSTEM && shading_toggle_) {
        ps_->render(ps_->state());
        /*
        const int gridSize = 400;
        const auto& vertices = ps_->state();

        const auto& springs = ps_->springs();

        
        vector<Vertex> positions(400);
        for (auto i = 0u; i < 400; ++i) {
            positions[i].position = ClothSystem::position(vertices, i);
        }

        glPointSize(5.0f);

        glBindBuffer(GL_ARRAY_BUFFER, gl_.vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * positions.size(), positions.data(), GL_DYNAMIC_DRAW);

        glUseProgram(gl_.shader_program);
        glUniformMatrix4fv(gl_.world_to_clip_uniform, 1, GL_FALSE, worldToClip.data());

        glBindVertexArray(gl_.mesh_vao);

        glDrawElements(GL_TRIANGLES, (int)positions.size(), GL_UNSIGNED_INT, 0);
        // unbind.
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        */
        
    }    
    glUseProgram(0);

    Im3d_EndFrame();
}

void App::handleDrop(GLFWwindow* window, int count, const char** paths) {}
void App::handleKeypress(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        static const float rot_incr = 1.5;

        if (key == GLFW_KEY_HOME)
            camera_rotation_angle_ -= 0.05 * EIGEN_PI;
        else if (key == GLFW_KEY_END)
            camera_rotation_angle_ += 0.05 * EIGEN_PI;
        else if (key == GLFW_KEY_R)
            ps_->reset();
        else if (key == GLFW_KEY_1)
        {
            ps_ = &simple_system_;
            ps_type_ = SIMPLE_SYSTEM;
            ps_->reset();
        }
        else if (key == GLFW_KEY_2)
        {
            ps_ = &spring_system_;
            ps_type_ = SPRING_SYSTEM;
            ps_->reset();
        }
        else if (key == GLFW_KEY_3)
        {
            ps_ = &pendulum_system_;
            ps_type_ = PENDULUM_SYSTEM;
            ps_->reset();
        }
        else if (key == GLFW_KEY_4)
        {
            ps_ = &cloth_system_;
            ps_type_ = CLOTH_SYSTEM;
            ps_->reset();
        }
        else if (key == GLFW_KEY_5)
        {
            ps_ = &sprinkler_system_;
            ps_type_ = SPRINKLER_SYSTEM;
            ps_->reset();
        }
        else if (key == GLFW_KEY_F1)
            integrator_ = EULER_INTEGRATOR;
        else if (key == GLFW_KEY_F2)
            integrator_ = TRAPEZOID_INTEGRATOR;
        else if (key == GLFW_KEY_F3)
            integrator_ = MIDPOINT_INTEGRATOR;
        else if (key == GLFW_KEY_F4)
            integrator_ = RK4_INTEGRATOR;
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

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        camera_rotation_angle_ += 0.01f * (xpos - old_x_pos);

    old_x_pos = xpos;
    old_y_pos = ypos;
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
