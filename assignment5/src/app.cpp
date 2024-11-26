// Include libraries
#include "glad/gl_core_33.h"                // OpenGL
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>             // Window manager
#include <imgui.h>                  // GUI Library
#include <imgui_impl_glfw.h>
#include "imgui_impl_opengl3.h"

#include "im3d.h"
#include "im3d_math.h"
#include "implot.h"

#include <Eigen/Dense>              // Linear algebra
#include <Eigen/Geometry>

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "fmt/core.h"
#include "Utils.h"

using namespace std;        // enables writing "string" instead of string, etc.
using namespace Eigen;      // enables writing "Vector3f" instead of "Eigen::Vector3f", etc.

#include "object.h"
#include "camera.h"
#include "film.h"
#include "ray_tracer.h"
#include "app.h"

#include <cassert>
#include <cstddef>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <type_traits>
#include <filesystem>

#ifndef CS3100_TTF_PATH
#define CS3100_TTF_PATH "roboto_mono.ttf"
#endif

//------------------------------------------------------------------------
// Static data members

App* App::static_instance_ = 0; // The single instance we allow of App

GLFWkeyfun          App::default_key_callback_          = nullptr;
GLFWmousebuttonfun  App::default_mouse_button_callback_ = nullptr;
GLFWcursorposfun    App::default_cursor_pos_callback_   = nullptr;
GLFWdropfun         App::default_drop_callback_         = nullptr;
GLFWscrollfun       App::default_scroll_callback_       = nullptr;

//------------------------------------------------------------------------
// Defined in main.cpp

shared_ptr<Image4f> render(RayTracer& rt, SceneParser& scene, const Args& args, bool parallelize);

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

void App::run()
{
    // Warn about cwd problems
    filesystem::path cwd = filesystem::current_path();
    if (!filesystem::is_directory(cwd / "assets")) {
        cout << fmt::format("Current working directory \"{}\" does not contain an \"assets\" folder.\n", cwd.string())
            << "Make sure the executable gets run relative to the project root.\n";
        return;
    }

    //static_assert(is_standard_layout<Vertex>::value, "struct Vertex must be standard layout to use offsetof");

    // Initialize GLFW
    if (!glfwInit())
        fail("glfwInit() failed");

    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_REFRESH_RATE, 0);

    // Create a windowed mode window and its OpenGL context
    window_ = glfwCreateWindow(1920, 1080, "CS-C3100 Computer Graphics, Assignment 5", NULL, NULL);
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

    loadSceneDialog();

    setUIScale(1.5f);

    vector<string> vecStatusMessages;

    static array<const char*, 3> filter_list = { "Box", "Tent", "Gaussian" };
    static array<Args::ReconstructionFilterType, 3> name2filter = { Args::Filter_Box, Args::Filter_Tent, Args::Filter_Gaussian };
    static map<Args::ReconstructionFilterType, int> filter2index;
    filter2index[Args::Filter_Box] = Args::Filter_Box;
    filter2index[Args::Filter_Tent] = Args::Filter_Tent;
    filter2index[Args::Filter_Gaussian] = Args::Filter_Gaussian;

    static array<const char*, 3> pattern_list = { "Regular", "Uniform random", "Jittered random" };
    static array<Args::SamplePatternType, 3> name2pattern = { Args::Pattern_Regular, Args::Pattern_UniformRandom, Args::Pattern_JitteredRandom };
    static map<Args::SamplePatternType, int> pattern2index;
    pattern2index[Args::Pattern_Regular] = Args::Pattern_Regular;
    pattern2index[Args::Pattern_UniformRandom] = Args::Pattern_UniformRandom;
    pattern2index[Args::Pattern_JitteredRandom] = Args::Pattern_JitteredRandom;

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
        glfwGetFramebufferSize(window_, &width, &height);
        int render_width = width - gui_width_*xscale;

        ImGui::SetNextWindowPos(ImVec2(gui_width_, 0));
        ImGui::SetNextWindowSize(ImVec2(render_width, height));
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Render surface", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);

        // Set render area window
        glViewport(gui_width_*xscale, 0, render_width, height);

        // Clear screen.
        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (display_results_)
            ImGui::Image((ImTextureID)(intptr_t)gl_texture_, ImVec2(render_width/xscale, height/yscale));
        else
            render(render_width, height, vecStatusMessages);

        ImGui::End();

        ImGui::PopStyleVar();
        
        //{
        //    vecStatusMessages.push_back(fmt::format("Display scale {}x, {}x", xscale, yscale));
        //    vecStatusMessages.push_back(fmt::format("Viewport {}x{}", width, height));
        //}

        // Begin GUI window
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(gui_width_, height));
        ImGui::SetNextWindowBgAlpha(1.0f);
        if (ImGui::Begin("Controls", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
        {
            if (ImGui::Button("Ray trace (ENTER)"))
                rayTrace(false);
            ImGui::SameLine();
            ImGui::Checkbox("Display results (SPACE)", &display_results_);

            if (ImGui::TreeNodeEx("Render options", ImGuiTreeNodeFlags_DefaultOpen))
            {
                int start_x = 100;
                int width = 250;

                ImGui::SetCursorPosX(start_x);
                ImGui::SetNextItemWidth(width);
                ImGui::Checkbox("Parallelize rendering", &parallelize_);

                // render resolution
                int resolution_factor_log2 = int(-log2(downscale_factor_));
                string factor = fmt::format("1/{}x", 1 << (-resolution_factor_log2));
                ImGui::SetCursorPosX(start_x);
                ImGui::SetNextItemWidth(width);
                ImGui::SliderInt("Resolution", &resolution_factor_log2, -5, 0, factor.c_str());
                downscale_factor_ = 1 << (-resolution_factor_log2);

                ImGui::SetCursorPosX(start_x);
                ImGui::SetNextItemWidth(width);
                ImGui::SliderInt("Bounces", &args_.bounces, 0, 10);

                // Samples per pixel. Only allow perfect squares (1, 4, 9, 16, 25, ...) etc.
                // in order not to break the uniform and jittered samplers.
                int sqrt_samples_per_pixel = int(sqrtf(args_.samples_per_pixel));
                string spp = fmt::format("{}", sqrt_samples_per_pixel * sqrt_samples_per_pixel);
                ImGui::SetCursorPosX(start_x);
                ImGui::SetNextItemWidth(width);
                ImGui::SliderInt("Samples/pixel", &sqrt_samples_per_pixel, 1, 16, spp.c_str());
                args_.samples_per_pixel = sqrt_samples_per_pixel * sqrt_samples_per_pixel;

                // filter
                int selected_filter = filter2index[args_.reconstruction_filter];
                ImGui::SetCursorPosX(start_x);
                ImGui::SetNextItemWidth(width);
                if (ImGui::Combo("Filter", &selected_filter, filter_list.data(), filter_list.size()))
                {
                    args_.reconstruction_filter = name2filter[selected_filter];
                    switch (args_.reconstruction_filter)
                    {
                    case Args::Filter_Box:
                        args_.filter_radius = 0.5f;
                        break;
                    case Args::Filter_Tent:
                        args_.filter_radius = 1.0f;
                        break;
                    case Args::Filter_Gaussian:
                        args_.filter_radius = 0.5f;
                        break;
                    }
                }

                // sample pattern
                int selected_pattern = pattern2index[args_.sampling_pattern];
                ImGui::SetCursorPosX(start_x);
                ImGui::SetNextItemWidth(width);
                if (ImGui::Combo("Sample pattern", &selected_pattern, pattern_list.data(), pattern_list.size()))
                    args_.sampling_pattern = name2pattern[selected_pattern];

                if (ImGui::Button("<"))
                    args_.random_seed = max(0, args_.random_seed - 1);
                ImGui::SameLine(50);
                if (ImGui::Button(">"))
                    args_.random_seed = min(255, args_.random_seed + 1);
                ImGui::SameLine(start_x);
                ImGui::SetNextItemWidth(width);
                ImGui::SliderInt("Random seed", &args_.random_seed, 0, 256);

                ImGui::SetCursorPosX(start_x);
                ImGui::SetNextItemWidth(width);
                ImGui::Checkbox("Shadows", &args_.shadows);

                ImGui::SetCursorPosX(start_x);
                ImGui::SetNextItemWidth(width);
                ImGui::Checkbox("Transparent shadows", &args_.transparent_shadows);

                ImGui::SetCursorPosX(start_x);
                ImGui::SetNextItemWidth(width);
                ImGui::Checkbox("Shade backfaces", &args_.shade_back);

                ImGui::SetCursorPosX(start_x);
                ImGui::SetNextItemWidth(width);
                ImGui::Checkbox("Show UV", &args_.display_uv);

                ImGui::TreePop();
            }

            // Draw the status messages in the current list
            // Note that you can add them from inside render(...)
            vecStatusMessages.push_back(fmt::format("Application average {:.3f} ms/frame ({:.1f} FPS)", 1000.0f / io_->Framerate, io_->Framerate));
            //vecStatusMessages.push_back("Home/End and mouse drag rotate camera");
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

    scene_.reset(new SceneParser());
}

void App::loadSceneDialog()
{
    string filename = fileOpenDialog("Scene specification", "txt");
    if (!filename.empty())
    {
        scene_.reset(new SceneParser(filename));
        scene_camera_rotation_ = scene_->getCamera()->getOrientation();

        Vector3f direction = scene_camera_rotation_.col(2).head(3);

        camera_position_ = scene_->getCamera()->getCenter();
        camera_rotation_ = Vector2f::Zero();

        if (scene_->getCamera()->isOrtho())
        {
            ortho_size_ = ((OrthographicCamera*)scene_->getCamera().get())->getSize();
            camera_type_ = SceneParser::Camera_Orthographic;
        }
        else
        {
            fov_ = ((PerspectiveCamera*)scene_->getCamera().get())->getFov();
            camera_type_ = SceneParser::Camera_Perspective;
        }
    }
    display_results_ = false;
}

Matrix4f App::getCamera()
{
    Matrix3f Rx(AngleAxis<float>(camera_rotation_(1), Vector3f(1.0f, 0.0f, 0.0f)));
    Matrix3f Ry(AngleAxis<float>(camera_rotation_(0), Vector3f(0.0f, 1.0f, 0.0f)));

    Matrix3f rot = Rx * Ry;
    rot = scene_camera_rotation_.transpose() * rot * scene_camera_rotation_ * scene_camera_rotation_;

    Matrix4f C = Matrix4f::Identity();
    C.block(0, 0, 3, 3) = rot;

    camera_position_ += C.transpose().block(0, 0, 3, 3) * camera_velocity_ * camera_speed_;

    Matrix4f T = Matrix4f::Identity();
    T.block(0, 3, 3, 1) = -camera_position_;

    C = C * T;
    
    return C;
}

void App::copyCamera()
{
    Matrix4f C = getCamera();

    auto c = scene_->getCamera();
    if (c == nullptr || c->isOrtho() != (camera_type_ == SceneParser::Camera_Orthographic))
    {
        if (camera_type_ == SceneParser::Camera_Orthographic)
            scene_->setCamera(make_shared<OrthographicCamera>(Vector3f::Zero(), Vector3f::Zero(), Vector3f::Zero(), .0f));
        else
            scene_->setCamera(make_shared<PerspectiveCamera>(Vector3f::Zero(), Vector3f::Zero(), Vector3f::Zero(), .0f));
    }

    scene_->getCamera()->setOrientation(C.block(0, 0, 3, 3).transpose());
    scene_->getCamera()->setCenter(camera_position_);

    if (scene_->getCamera()->isOrtho())
        ((OrthographicCamera*)scene_->getCamera().get())->setSize(ortho_size_);
    else
        ((PerspectiveCamera*)scene_->getCamera().get())->setFov(fov_);
}

Args App::get_args()
{	
    Args args(args_);

    int w, h;
    glfwGetWindowSize(window_, &w, &h);

    args.width =  (w - gui_width_) / downscale_factor_;
    args.height = h / downscale_factor_;
    args.output_file = "debug.png";
    args.show_progress = true;

    return args;
}

void App::initRendering()
{
    glAssert(glGenTextures(1, &gl_texture_));
    // Setup filtering parameters for display
    glAssert(glBindTexture(GL_TEXTURE_2D, gl_texture_));
    glAssert(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    glAssert(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
}

void App::rayTrace(bool debug_current_pixel)
{
    copyCamera();

    SceneParser& local_scene(*scene_.get());
    auto args = get_args();
    auto tr = RayTracer(local_scene, args, debug_current_pixel);

    // If there is no scene, just display the UV coords
    if (!local_scene.getGroup())
        args.display_uv = true;

    if (debug_current_pixel)
    {
        int width, height;
        glfwGetWindowSize(window_, &width, &height);
        float fAspect = float(width-gui_width_) / height;
        Vector2f ray_xy = Camera::normalizedImageCoordinateFromPixelCoordinate(Vector2f(mouse_pos_x_-gui_width_, mouse_pos_y_), Vector2i(width-gui_width_, height));
        Ray r = scene_->getCamera()->generateRay(ray_xy, fAspect);
        Hit hit;
        float tmin = scene_->getCamera()->getTMin();
        Vector3f sample_color = tr.traceRay(r, tmin, args.bounces, 1.0f, hit, Vector3f{ 1.0f, 1.0f, 1.0f });
        debug_rays_ = tr.debug_rays;
        display_results_ = false;
    }
    else
    {
        result_image_ = ::render(tr, local_scene, args, parallelize_);
        shared_ptr<Image4u8> u8img = result_image_->to_uint8();

        Vector2i wh = u8img->getSize();
        glAssert(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));
        glAssert(glBindTexture(GL_TEXTURE_2D, gl_texture_));
        glAssert(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, wh(0), wh(1), 0, GL_RGBA, GL_UNSIGNED_BYTE, u8img->data()));
        glAssert(glBindTexture(GL_TEXTURE_2D, 0));
    }
}

void App::render(int width, int height, vector<string>& vecStatusMessages)
{
    // Enable depth testing.
    glEnable(GL_DEPTH_TEST);

    // Our camera orbits around origin at a fixed distance.

    auto C = getCamera();

    float fAspect = float(width) / height;

    Matrix4f P = Matrix4f::Identity();
    const float fNear = 0.02f, fFar = 80.0f;
    Vector2i window_size(width, height);
    if (camera_type_ == SceneParser::Camera_Perspective)
    {
        // Simple perspective.
        const float near_width = 1.0f / tan(fov_ / 2.0f);
        P <<RowVector4f{ near_width/fAspect,               0.0f,                            0.0f,                               0.0f },
            RowVector4f{       0.0f, near_width,                            0.0f,                               0.0f },
            RowVector4f{       0.0f,               0.0f, (fFar + fNear) / (fFar - fNear), -2 * fFar * fNear / (fFar - fNear) },
            RowVector4f{       0.0f,               0.0f,                            1.0f,                               0.0f };
    }
    else if (camera_type_ == SceneParser::Camera_Orthographic)
    {
        P.col(0) = Vector4f(2.0f/ortho_size_/fAspect, 0, 0, 0);
        P.col(1) = Vector4f(0, 2.0f/ortho_size_, 0, 0);
        P.col(2) = Vector4f(0, 0, 2.0f / (fFar - fNear), .0f);
        P.col(3) = Vector4f(0, 0, -(fFar + fNear) / (fFar - fNear), 1);
    }

    double mousex, mousey;
    glfwGetCursorPos(window_, &mousex, &mousey);
    Im3d_NewFrame(window_, width, height, C, P, 0.01f, mousex, mousey);

    if (scene_ != nullptr)
    {
        shared_ptr<GroupObject> group = scene_->getGroup();
        if(group != nullptr)
            group->preview_render(Matrix4f::Identity());
    }

    glUseProgram(0);

    Im3d::BeginLines();
    for (const auto& r : debug_rays_)
    {
        Im3d::SetColor(r.color(0), r.color(1), r.color(2), 0.8f);
        Im3d::Vertex(r.origin);
        Im3d::Vertex(Vector3f(r.origin + r.offset));
        Im3d::SetColor(0.0f, 0.0f, 1.0f, 0.8f);
        Im3d::Vertex(Vector3f(r.origin + r.offset));
        Im3d::Vertex(Vector3f(r.origin + r.offset + r.normal_at_offset * 0.1f));
    }
    Im3d::End();

    Im3d_EndFrame();
}


void App::handleDrop(GLFWwindow* window, int count, const char** paths) {}
void App::handleKeypress(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_ENTER)
            rayTrace(false);
        else if (key == GLFW_KEY_SPACE)
            display_results_ = !display_results_;
        else if (key == GLFW_KEY_E)
            rayTrace(true);
        else if (key == GLFW_KEY_L)
            loadSceneDialog();
        else if (key == GLFW_KEY_W)
            camera_velocity_(2) = +1.0f;
        else if (key == GLFW_KEY_S)
            camera_velocity_(2) = -1.0f;
        else if (key == GLFW_KEY_A)
            camera_velocity_(0) = -1.0f;
        else if (key == GLFW_KEY_D)
            camera_velocity_(0) = +1.0f;
        else if (key == GLFW_KEY_R)
            scene_camera_rotation_.setIdentity();
    }
    else if (action == GLFW_RELEASE)
    {
        if (key == GLFW_KEY_W)
            camera_velocity_(2) = 0.0f;
        else if (key == GLFW_KEY_S)
            camera_velocity_(2) = 0.0f;
        else if (key == GLFW_KEY_A)
            camera_velocity_(0) = 0.0f;
        else if (key == GLFW_KEY_D)
            camera_velocity_(0) = 0.0f;

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

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        camera_rotation_(0) += 0.001f * (xpos - mouse_pos_x_);
        camera_rotation_(1) += 0.001f * (ypos - mouse_pos_y_);
    }

    mouse_pos_x_ = xpos;
    mouse_pos_y_ = ypos;
}

void App::handleScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    if ( yoffset > 0 )
        camera_speed_ *= 1.2f * yoffset;
    if (yoffset < 0)
        camera_speed_ /= 1.2f * -yoffset;
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

    if ( !ImGui::IsAnyItemActive() )
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
