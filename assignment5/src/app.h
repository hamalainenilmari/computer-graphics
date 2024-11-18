#pragma once

#include "scene_parser.h"
#include <vector>
#include <memory>

#include "ray_tracer.h"

#include "args.h"

class App
{
private:
public:
    App();
    void            run();

private:

    Matrix4f        getCamera();
    void			copyCamera();

private:
                    App             (const App&); // forbid copy
    App&            operator=       (const App&); // forbid assignment

    Args			get_args();

private:
    GLFWwindow*		window_     = nullptr;
    ImGuiIO*        io_         = nullptr;

    void            loadSceneDialog();
    void            initRendering();
    void            render(int width, int height, vector<string>& vecStatusMessages);
    void            rayTrace(bool debug_current_pixel); // trace image with current settings, update result_image_ if not debugging

    Args            args_;

    std::unique_ptr<SceneParser>	scene_;
    bool                            display_results_    = false;
    bool                            parallelize_        = false;    // Change this to true if desired
    int                             downscale_factor_   = 8;

    SceneParser::ProjectionType     camera_type_        = SceneParser::Camera_Perspective;

    Vector3f        camera_position_        = Vector3f(0.0f, 0.0f, 2.0f);
    Vector3f        camera_velocity_        = Vector3f::Zero(); // in the local coordinates of the camera
    Vector2f        camera_rotation_        = Vector2f::Zero();
    Matrix3f        scene_camera_rotation_  = Matrix3f::Identity();
    float			fov_                    = 1.0f;
    float           ortho_size_             = 10.0f;

    int             gui_width_              = 512;
    float           camera_speed_           = 0.001f;
    double          mouse_pos_x_            = 0.0;
    double          mouse_pos_y_            = 0.0;
    bool			mouse_right_down_       = false;

    shared_ptr<Image4f> result_image_       = nullptr;
    GLuint              gl_texture_         = 0;

    vector<RaySegment> debug_rays_;

    // ------------------------------------------
    static GLFWkeyfun           default_key_callback_;
    static GLFWmousebuttonfun   default_mouse_button_callback_;
    static GLFWcursorposfun     default_cursor_pos_callback_;
    static GLFWdropfun          default_drop_callback_;
    static GLFWscrollfun        default_scroll_callback_;

    void				handleKeypress(GLFWwindow* window, int key, int scancode, int action, int mods);
    void				handleMouseButton(GLFWwindow* window, int button, int action, int mods);
    void				handleMouseMovement(GLFWwindow* window, double xpos, double ypos);
    void				handleScroll(GLFWwindow* window, double xoffset, double yoffset);
    void				handleDrop(GLFWwindow* window, int count, const char** paths);

    // Controls font and UI element scaling
    void                increaseUIScale();
    void                decreaseUIScale();
    void                setUIScale(float scale);
    void                loadFont(const char* name, float sizePixels);

    ImFont*             font_               = nullptr; // updated when size changes
    float               ui_scale_           = 1.0f;
    bool                font_atlas_dirty_   = false;

    // global instance of the App class (Singleton design pattern)
    // Needed for the GLFW callbacks
    static App* static_instance_;

public:
    // These need to be public, as they're assigned to the GLFW
    // TODO move these to a base class
    static void			key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void			mousebutton_callback(GLFWwindow* window, int button, int action, int mods);
    static void			cursorpos_callback(GLFWwindow* window, double xpos, double ypos);
    static void			scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    static void			drop_callback(GLFWwindow* window, int count, const char** paths);
    static void         error_callback(int error, const char* description);

};

