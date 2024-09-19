#pragma once

// Include libraries
#include "glad/gl.h"                // OpenGL
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>             // Window manager
#include <imgui.h>                  // GUI Library
#include <imgui_impl_glfw.h>
#include "imgui_impl_opengl3.h"
// This is a precaution we take against accidental misuses of the Eigen library,
// as documented here: https://eigen.tuxfamily.org/dox-devel/group__TopicPassingByValue.html 
// Disabling vectorization this way *should* make careless code work,
// but it's better really not to ever pass Eigen objects by value.
#define EIGEN_MAX_STATIC_ALIGN_BYTES 0
#include <Eigen/Dense>              // Linear algebra
#include <Eigen/Geometry>
#include <string>

//------------------------------------------------------------------------

#include "ShaderProgram.h"
#include "Timer.h"

//------------------------------------------------------------------------

using namespace std;        // enables writing "string" instead of std::string, etc.
using namespace Eigen;      // enables writing "Vector3f" instead of "Eigen::Vector3f", etc.

//------------------------------------------------------------------------

class App
{
public:
                        App();		// constructor
                        ~App();	    // destructor

    void				run();

    // Structure for holding vertex data.
    // Defined publicly so that can be used in the State struct.
    struct Vertex
    {
        Vector3f position;
        Vector3f normal;

        static Vertex Zero() { return Vertex{ Vector3f::Zero(), Vector3f::Zero() }; }
    };

private:
                        App(const App&) {};		        // forbid copy
                        App& operator=(const App&) {}	// forbid assignment

    void				initRendering();
    void				render(int window_width, int window_height, vector<string>& vecStatusMessages);

    void				showObjLoadDialog();
    vector<Vertex>		loadObjFileModel(string filename);
    void				loadFont(const char* name, float sizePixels);

    void				uploadGeometryToGPU(const vector<Vertex>& vertices);

    static vector<Vertex>	loadExampleModel();
    static vector<Vertex>	loadIndexedDataModel();
    static vector<Vertex>	loadGeneratedConeModel();
    static vector<Vertex>	unpackIndexedData(const vector<Vector3f>& positions,
                                              const vector<Vector3f>& normals,
                                              const vector<array<unsigned, 6>>& faces);
    

    // Controls font and UI element scaling
    void                increaseUIScale();
    void                decreaseUIScale();
    void                setUIScale(float scale);
    float               ui_scale_ = 1.0f;
    int                 fov_scale = 1;
    bool                font_atlas_dirty_ = false;

    static GLFWkeyfun          default_key_callback_;
    static GLFWmousebuttonfun  default_mouse_button_callback_;
    static GLFWcursorposfun    default_cursor_pos_callback_;
    static GLFWdropfun         default_drop_callback_;

    struct glGeneratedIndices
    {
        GLuint static_vao, dynamic_vao;
        GLuint shader_program;
        GLuint static_vertex_buffer, dynamic_vertex_buffer;
        GLuint model_to_world_uniform, world_to_clip_uniform, shading_toggle_uniform;
    };

    GLFWwindow*			window_;
    Timer               timer_;
    ImGuiIO*			io_;
    ImFont*				font_; // updated when size changes

    size_t				vertex_count_;
    bool				model_changed_;
    bool				shading_toggle_;
    bool				shading_mode_changed_;
    bool                auto_rotate_ = false;

    glGeneratedIndices	gl_;

    float				camera_rotation_angle_;

    // global instance of the App class (Singleton design pattern)
    // Needed for the GLFW key and mouse callbacks
    static App*			static_instance_;

    // YOUR CODE HERE (R1)
    // Add a class member to store the current translation.
    Vector3f currentTranslation = Vector3f::Zero();

public:
    // These need to be public, as they're assigned to the GLFW
    static void			key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void			mousebutton_callback(GLFWwindow* window, int button, int action, int mods);
    static void			cursorpos_callback(GLFWwindow* window, double xpos, double ypos);
    static void			drop_callback(GLFWwindow* window, int count, const char** paths);
    static void         error_callback(int error, const char* description);

private:
    void				handleKeypress(GLFWwindow* window, int key, int scancode, int action, int mods);
    void				rotateCamera();
    void				handleMouseButton(GLFWwindow* window, int button, int action, int mods);
    void				handleMouseMovement(GLFWwindow* window, double xpos, double ypos);
    void				handleDrop(GLFWwindow* window, int count, const char** paths);

};

//------------------------------------------------------------------------


// http://stackoverflow.com/questions/2270726/how-to-determine-the-size-of-an-array-of-strings-in-c
template <typename T, size_t N>
char(&static_sizeof_array(T(&)[N]))[N]; // declared, not defined
#define SIZEOF_ARRAY( x ) sizeof(static_sizeof_array(x))

//------------------------------------------------------------------------
