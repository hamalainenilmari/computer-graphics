#pragma once

// Include libraries
#include "glad/gl_core_33.h"                // OpenGL
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>             // Window manager
#include <imgui.h>                  // GUI Library
#include <imgui_impl_glfw.h>
#include "imgui_impl_opengl3.h"

#include <Eigen/Dense>              // Linear algebra
#include <Eigen/Geometry>

#include <string>
#include <vector>
#include <memory>

using namespace std;        // enables writing "string" instead of string, etc.
using namespace Eigen;      // enables writing "Vector3f" instead of "Eigen::Vector3f", etc.

#include "Timer.h"      // From shared sources -->
#include "ShaderProgram.h"
#include "Utils.h"

#include "skeleton.h"


// Structure for holding vertex data.
// PNC stands for Position, Normal, Color
struct VertexPNC
{
    Vector3f position   = Vector3f::Zero();
    Vector3f normal     = Vector3f::Zero();
    Vector3f color      = Vector3f::Zero();
};

struct WeightedVertex
{
    Vector3f position   = Vector3f::Zero();
    Vector3f normal     = Vector3f::Zero();
    Vector3f color      = Vector3f::Zero();	
    int		joints[WEIGHTS_PER_VERTEX];
    float	weights[WEIGHTS_PER_VERTEX];
};

struct glGeneratedIndices
{
    // Shader programs
    GLuint simple_shader, ssd_shader;

    // Vertex array objects
    GLuint simple_vao, ssd_vao;

    // Buffers
    GLuint simple_vertex_buffer, ssd_vertex_buffer;

    // simple_shader uniforms
    GLint simple_world_to_clip_uniform, simple_shading_mix_uniform;

    // ssd_shader uniforms
    GLint ssd_world_to_clip_uniform, ssd_shading_mix_uniform, ssd_transforms_uniform;
};

class App
{
private:
    enum DrawMode
    {
        DrawMode_Skeleton,
        DrawMode_SSD_CPU,
        DrawMode_SSD_GPU
    };

public:
                    App             (void);
                    ~App            (void);

    void			run();

private:
    void			initRendering		(void);
    void			render				(int window_width, int window_height, vector<string>& vecStatusMessages);
    void			renderSkeleton		(vector<string>& vecStatusMessages);

    void			loadModel			(const string& filename);
    void			loadCharacter		(const string& filename);
    
    vector<WeightedVertex>	loadAnimatedMesh		(string namefile, string mesh_file, string attachment_file);

    void					computeSSD				(const vector<WeightedVertex>& source, vector<VertexPNC>& dest);
private:
                    App             (const App&) {} // forbid copy
    App&            operator=       (const App&) {} // forbid assignment

private:
    GLFWwindow*			window_ = nullptr;
    ImGuiIO*			io_		= nullptr;

    // CommonControls	common_ctrl_;

    DrawMode			draw_mode_ = DrawMode_Skeleton;
    bool                draw_joint_frames_ = true;
    string				filename_;
    bool				shading_toggle_ = false;
    bool				shading_mode_changed_ = false;
    vector<Vector3f>	joint_colors_;

    glGeneratedIndices	gl_;

    vector<WeightedVertex> weighted_vertices_;
    
    float				camera_rotation_ = float(EIGEN_PI);
    float				camera_distance_ = 3.0f;
    float				camera_height_ = 1.0f;

    Skeleton			skel_;
    unsigned			selected_joint_ = 0;

    bool				animation_mode_ = false;
    float               animation_current_time_ = 0.0f;
    int                 animation_speed_exponent_ = 0;

    enum VertexShaderAttributeLocations
    {
        ATTRIB_POSITION = 0,
        ATTRIB_NORMAL = 1,
        ATTRIB_COLOR = 2,
        ATTRIB_JOINTS1 = 3,
        ATTRIB_JOINTS2 = 4,
        ATTRIB_WEIGHTS1 = 5,
        ATTRIB_WEIGHTS2 = 6
    };

    // ------------------------------------------
    static GLFWkeyfun           default_key_callback_;
    static GLFWmousebuttonfun   default_mouse_button_callback_;
    static GLFWcursorposfun     default_cursor_pos_callback_;
    static GLFWdropfun          default_drop_callback_;

    void				handleKeypress(GLFWwindow* window, int key, int scancode, int action, int mods);
    void				handleMouseButton(GLFWwindow* window, int button, int action, int mods);
    void				handleMouseMovement(GLFWwindow* window, double xpos, double ypos);
    void				handleDrop(GLFWwindow* window, int count, const char** paths);

    // Controls font and UI element scaling
    ImFont* font_       = nullptr; // updated when size changes
    void                increaseUIScale();
    void                decreaseUIScale();
    void                setUIScale(float scale);
    float               ui_scale_ = 1.0f;
    bool                font_atlas_dirty_ = false;
    
    void                loadFont(const char* name, float sizePixels);

    // global instance of the App class (Singleton design pattern)
    // Needed for the GLFW callbacks
    static App* static_instance_;

public:
    // These need to be public, as they're assigned to the GLFW
    // TODO move these to a base class
    static void			key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void			mousebutton_callback(GLFWwindow* window, int button, int action, int mods);
    static void			cursorpos_callback(GLFWwindow* window, double xpos, double ypos);
    static void			drop_callback(GLFWwindow* window, int count, const char** paths);
    static void         error_callback(int error, const char* description);
};
