// Include libraries
#include "glad/gl_core_33.h"                // OpenGL
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>             // Window manager
#include <imgui.h>                  // GUI Library
#include <imgui_impl_glfw.h>
#include "imgui_impl_opengl3.h"

#include "tiny_obj_loader.h"

#include "im3d.h"
#include "im3d_math.h"

#include <Eigen/Dense>              // Linear algebra
#include <Eigen/Geometry>

using namespace std;        // enables writing "string" instead of string, etc.
using namespace Eigen;      // enables writing "Vector3f" instead of "Eigen::Vector3f", etc.

#include <iostream>

#include "ShaderProgram.h"
#include "app.h"
#include "shadowmap.h"

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


Matrix4f LightSource::getPosToLightClip(const Vector3f& scene_bb_min, const Vector3f& scene_bb_max) const
{
	// YOUR SHADOWS HERE
	// You'll need to construct the matrix that sends world space points to
	// points in the light source's clip space. You'll need to somehow use
	// m_xform, which describes the light source pose. Since we are using directional
    // lights, your projection should be orthographic (not perspective).
	return Matrix4f::Identity(); // placeholder
}

void LightSource::visualize() const
{
	Im3d::BeginLines();
	Im3d::SetColor(Im3d::Color_Red);
	Im3d::Vertex((Vector3f(m_xform.col(3).head(3))));
	Im3d::Vertex((Vector3f(m_xform.col(3).head(3) + m_xform.col(0).head(3))));
	Im3d::SetColor(Im3d::Color_Green);;
	Im3d::Vertex((Vector3f(m_xform.col(3).head(3))));
	Im3d::Vertex((Vector3f(m_xform.col(3).head(3) + m_xform.col(1).head(3))));
	Im3d::SetColor(Im3d::Color_Blue);
	Im3d::Vertex((Vector3f(m_xform.col(3).head(3))));
	Im3d::Vertex((Vector3f(m_xform.col(3).head(3) + m_xform.col(2).head(3))));
	Im3d::End();
}

void LightSource::renderShadowMap(int numTriangles, ShaderProgram* pShader, ShadowMapContext& sm, const Vector3f& scene_bb_min, const Vector3f& scene_bb_max)
{
	// If this light source does not yet have an allocated OpenGL texture, then get one
	if (m_shadowMapTexture == 0)
		m_shadowMapTexture = sm.allocateDepthTexture();

	// Attach the shadow rendering state (off-screen buffers, render target texture)
	sm.attach(m_shadowMapTexture);
	// Here's a trick: we can cull the front faces instead of the backfaces; this
	// moves the shadow map bias nastiness problems to the dark side of the objects
	// where they might not be visible.
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	
	// Clear the texture to maximum distance (1.0 in NDC coordinates)
	glClearColor(1.0f, 0, 0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	// Set the transformation matrix uniform
	pShader->setUniform("posToLightClip", getPosToLightClip(scene_bb_min, scene_bb_max));
	pShader->setUniform("posToLight", Matrix4f(m_xform.inverse()));

	renderSceneRaw(numTriangles, pShader);

	// Detach off-screen buffers
	glAssert(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}
//
//
//
///*************************************************************************************
// * Nasty OpenGL stuff begins here, you should not need to touch the remainder of this
// * file for the basic requirements.
// *
// */
//
void ShadowMapContext::setup(const Vector2i& resolution)
{
	m_resolution = resolution;

	free();	// Delete the existing stuff, if any
	
	glAssert(glGenRenderbuffers(1, &m_depthRenderbuffer));
    glAssert(glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderbuffer));
    glAssert(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_resolution(0), m_resolution(1)));

    glAssert(glGenFramebuffers(1, &m_framebuffer));
    glAssert(glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer));
    glAssert(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRenderbuffer));
	
	// We now have a framebuffer object with a renderbuffer attachment for depth.
	// We'll render the actual depth maps into separate textures, which are attached
	// to the color buffers upon rendering.

	// Don't attach yet.
    glAssert(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

GLuint ShadowMapContext::allocateDepthTexture()
{
	GLuint tex;

    glAssert(glGenTextures(1, &tex));
	glAssert(glBindTexture(GL_TEXTURE_2D, tex));
	glAssert(glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_resolution(0), m_resolution(1), 0, GL_RED, GL_FLOAT, NULL));
	glAssert(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	glAssert(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	glAssert(glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	glAssert(glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

	return tex;
}


void ShadowMapContext::attach(GLuint texture)
{
	if (m_framebuffer == 0)
		fail("Error: ShadowMapContext not initialized");

	glAssert(glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer));
	glAssert(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0));
	glAssert(glViewport(0, 0, m_resolution(0), m_resolution(1)));
}


void LightSource::renderSceneRaw(int numTriangles, ShaderProgram *prog)
{
	// Just draw the triangles without doing anything fancy; anything else should be done on the caller's side
    // VBO needs to be bound by caller

    glAssert(glDrawElements(GL_TRIANGLES, 3 * numTriangles, GL_UNSIGNED_INT, (void*)0));
}

