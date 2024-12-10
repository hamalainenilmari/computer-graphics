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

#include "ShaderProgram.h"
#include "Timer.h"
#include "image.h"
#include "camera.h"

// ----------------------------------------------------------------------------
// These have to be defined before inclusion of shadowmap.h

struct VertexPNT
{
    Vector3f position = Vector3f::Zero();
    Vector3f normal = Vector3f::Zero();
    Vector2f uv = Vector2f::Zero();
};

// Simple structure that contains a mesh where all triangles have the same material.
// Multi-material OBJ files are broken down to many such items.
struct SubMesh
{
    unsigned int            index_start;
    unsigned int            num_triangles;
    tinyobj::material_t     material;
};

#include "shadowmap.h"

// ----------------------------------------------------------------------------

class App
{
private:
    enum CullMode
    {
        CullMode_None = 0,
        CullMode_CW,
        CullMode_CCW,
    };

    struct glGeneratedIndices
    {
        GLuint shader;  // shader program for main view
        GLuint shadowMapShader;
        GLuint vao;     // vertex array object
        GLuint vertex_buffer;
        GLuint index_buffer;
    };

public:
                    App             (void);
    virtual         ~App            (void);
    void            run();

    //virtual void    readState       (StateDump& d);
    //virtual void    writeState      (StateDump& d) const;

private:
    void            waitKey         (void);
    void            renderFrame     ();
    void            renderScene     (const Matrix4f& worldToCamera, const Matrix4f& projection);
	void			loadScene		(const filesystem::path& fileName);
    void            loadMesh        (const filesystem::path& fileName);     // appends to m_meshes
    //void            saveMesh        (const filesystem::path& fileName);

	void			initRendering();
    void            compileShadowMapShader();
    string          loadPixelShader();
    void            loadAndCompileShaders(const string& vs, const string& ps);
    void            uploadToGPU(const filesystem::path& texturepath);
    void            render(int width, int height, vector<string>& vecStatusMessages);
	void			renderWithNormalMap				(const Matrix4f& cameraToWorld, const Matrix4f& projection);
	static Image3f	convertBumpToObjectSpaceNormal	(const Image1f& bump, void* pMesh, float alpha);

private:
                    App             (const App&); // forbidden
    App&            operator=       (const App&); // forbidden

private:
    GLFWwindow*     window_ = nullptr;
    ImGuiIO*        io_ = nullptr;

    Camera          camera_;

    Action              m_action;
    vector<SubMesh>     m_submeshes;
    vector<VertexPNT>   m_vertices;
    vector<Vector3i>    m_indices;
    map<string, GLint>  m_vertex_attribute_indices;
    string              m_vertex_shader_source;

    int                 m_viewportX = -1;
    int                 m_viewportY = -1;
    int                 m_viewportW = -1;
    int                 m_viewportH = -1;

    Vector3f            m_boundingbox_min = Vector3f(FLT_MAX, FLT_MAX, FLT_MAX);
    Vector3f            m_boundingbox_max = Vector3f(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    map<string, GLuint> m_textures;
    CullMode            m_cullMode = CullMode_None;
    bool                m_wireframe = false;

	shared_ptr<ShaderProgram>	    m_shaderProgram = nullptr;
    shared_ptr<ShaderProgram>	    m_shadowMapShader = nullptr;
    vector<string>                  m_shaderCompilationErrors;

    glGeneratedIndices gl_;
	int             m_renderMode = 0;
	bool				m_useNormalMap = true;
	bool				m_useDiffuseTexture = true;
	float              m_normalMapScale = 0.15f;
	bool				m_setDiffuseToZero = false;
	bool				m_setSpecularToZero = false;
    bool                m_overrideRoughness = false;
    float               m_roughness = 1e-4f;

    string          m_rayDumpFileName;
    int             m_numRays = 0;

	vector<pair<LightSource, ShadowMapContext>> m_lights;
	int             m_viewpoint = 0;
	Timer			m_lamptimer; // You can use this to move the lights
	bool			m_shadows = false;
	float			m_shadowEps = 0.015f;

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

    ImFont* font_ = nullptr; // updated when size changes
    float               ui_scale_ = 1.0f;
    bool                font_atlas_dirty_ = false;

    // global instance of the App class (Singleton design pattern)
    // Needed for the GLFW callbacks
    static App*         static_instance_;

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

