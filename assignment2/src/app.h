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

using namespace std;        // enables writing "string" instead of std::string, etc.
using namespace Eigen;      // enables writing "Vector3f" instead of "Eigen::Vector3f", etc.

#include "Timer.h"      // From shared sources -->
#include "ShaderProgram.h"
#include "Utils.h"
#include "camera.h"     // From this assignment-->
#include "curve.h"
#include "subdiv.h"

//------------------------------------------------------------------------

// Structure for holding vertex data.
// PNC stands for Position, Normal, Color
struct VertexPNC
{
    Vector3f position;
    Vector3f normal;
    Vector3f color;

    static VertexPNC Zero() { return VertexPNC{ Vector3f::Zero(), Vector3f::Zero(), Vector3f::Zero() }; }
};

//------------------------------------------------------------------------

class App
{
private:
    enum DrawMode
    {
        DrawMode_Curves,
        DrawMode_Subdivision
    };

public:
                        App(void);
                        ~App(void);

    void				run();

private:
                        App(const App&) = delete; // forbidden
                        App& operator=(const App&) = delete; // forbidden

    void render(int window_width, int window_height, vector<string>& vecStatusMessages);

    void arcballRotation(int end_x, int end_y);
    void setupViewportAndProjection    (int w, int h);
    void drawScene      (void);
    void initRendering  (void);
    void loadSWP        (const string& filename);
    void writeObjects   (const string& filename);
	void loadOBJ        (const string& filename);
    void renderCurves(void);
    void screenshot     (const string& name);
    void handleLoading();

    GLFWwindow*         window_                 = nullptr;
    ImGuiIO*            io_                     = nullptr;

    DrawMode            draw_mode_              = DrawMode_Curves;
    bool				draw_wireframe_         = false;
    bool				debug_subdivision_      = false;

	string				filename_;
    
    // This is the camera
    Camera              camera_;

    // These vectors store the control points, curves, and surfaces
	// that will end up being drawn.  In addition, parallel vectors
	// store the names for the curves and surfaces (as given by the files).
    vector<SplineCurve>	        spline_curves_;
    vector<vector<CurvePoint>>  tessellated_curves_;
    int                         tessellation_steps_ = 8;

    void tessellateCurves();

    enum VertexShaderAttributeLocations {
        ATTRIB_POSITION = 0,
        ATTRIB_NORMAL = 1,
        ATTRIB_COLOR = 2
    };

    struct glGeneratedIndices
    {
        GLuint shader_program;
        GLuint vao;
        GLuint vertex_buffer;
        GLuint index_buffer;
        GLuint world_to_view_uniform, view_to_clip_uniform, shading_toggle_uniform, camera_world_position_uniform;
    };

    glGeneratedIndices gl_;

	vector<unique_ptr<MeshWithConnectivity>>    subdivided_meshes_;
	int							                current_subdivision_level_ = 0;
    ShaderProgram*                              subdivision_shader_ = nullptr;

    void                increaseSubdivisionLevel();
    void                decreaseSubdivisionLevel();

    void                uploadGeometryToGPU(const MeshWithConnectivity& m);
    void                renderMesh(const MeshWithConnectivity& m, const Camera& cam, bool include_wireframe, int highlight_triangle) const;
    int                 pickTriangle(const MeshWithConnectivity& m, const Camera& cam, int window_width, int window_height, float mousex, float mousey) const;


public:
    // These need to be public, as they're assigned to the GLFW
    // TODO move these to a base class
    static void			key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void			mousebutton_callback(GLFWwindow* window, int button, int action, int mods);
    static void			cursorpos_callback(GLFWwindow* window, double xpos, double ypos);
    static void			drop_callback(GLFWwindow* window, int count, const char** paths);
    static void         error_callback(int error, const char* description);

private:
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
};
