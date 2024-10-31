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

#include "particle_system.h"

struct Vertex
{
	Vector3f position;
	Vector3f normal;
};

struct glGeneratedIndices
{
	GLuint point_vao, mesh_vao;
	GLuint shader_program;
	GLuint vertex_buffer;
	GLuint model_to_world_uniform, world_to_clip_uniform;
};

class App
{
private:
	enum ParticleSystemType
	{
		SIMPLE_SYSTEM,
		SPRING_SYSTEM,
		PENDULUM_SYSTEM,
		CLOTH_SYSTEM,
		//COMPUTE_CLOTH
	};
	enum IntegratorType
	{
		EULER_INTEGRATOR,
		TRAPEZOID_INTEGRATOR,
		MIDPOINT_INTEGRATOR,
		RK4_INTEGRATOR,
		//IMPLICIT_EULER_INTEGRATOR,
		//IMPLICIT_MIDPOINT_INTEGRATOR,
		//CRANK_NICOLSON_INTEGRATOR,
		//COMPUTE_CLOTH_INTEGRATOR
	};
public:
					App             (void);

	void			run();


private:
	void			initRendering();
	void			step();
	void			render(int window_width, int window_height, vector<string>& vecStatusMessages);

private:
					App             (const App&); // forbid copy
	App&            operator=       (const App&); // forbid assignment

private:
	GLFWwindow*			window_ = nullptr;
	ImGuiIO*			io_	= nullptr;

	bool				shading_toggle_ = false;

	glGeneratedIndices	gl_;

	float				camera_rotation_angle_	= 0.0f;

	ParticleSystemType	ps_type_ = SIMPLE_SYSTEM;
	IntegratorType		integrator_ = MIDPOINT_INTEGRATOR;
	ParticleSystem*		ps_ = &simple_system_;
	
	float				step_ = 0.0001f;
	int					steps_per_update_ = 1;
	SimpleSystem		simple_system_;
	SpringSystem		spring_system_;
	MultiPendulumSystem	pendulum_system_;
	ClothSystem			cloth_system_;

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
	ImFont* font_ = nullptr; // updated when size changes
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

