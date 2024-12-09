#pragma once

#include "Utils.h"

// Given an N-vector, return an N+1 -vector with a 1 added to last dimension
template<typename Scalar, int N>
inline Vector<Scalar, N + 1> promote(const Vector<Scalar, N>& v)
{
    Vector<Scalar, N + 1> result;
    result.block(0, 0, N, 1) = v;
    result[N] = 1.0f;
    return result;
}


// This is a technical thing that holds the off-screen buffers related
// to shadow map rendering.
class ShadowMapContext
{
public:
    ShadowMapContext() :
      m_depthRenderbuffer(0),
      m_framebuffer(0),
      m_resolution(256, 256)
        { }

    ~ShadowMapContext() { free(); }

    void setup(const Vector2i& resolution);
    void free() {};

    GLuint allocateDepthTexture();
    void attach(GLuint texture);
    bool ok() const { return m_framebuffer != 0; }

protected:
    GLuint m_depthRenderbuffer;
    GLuint m_framebuffer;
    Vector2i m_resolution;
};

class LightSource
{
public:
    LightSource() {}

    void			setStaticPosition(const Vector3f& p) { m_staticPosition = p; setPosition(p); }
    Vector3f			getStaticPosition(void)			{ return m_staticPosition; }

    Vector3f			getPosition(void) const			{ return m_xform.col(3).head(3); }
    void			setPosition(const Vector3f& p)		{ m_xform.col(3) = promote(p); }

    Matrix3f			getOrientation(void) const      { return m_xform.block(0, 0, 3, 3); }
    void			setOrientation(const Matrix3f& R)   { m_xform.block(0, 0, 3, 3) = R; }

    Vector3f			getNormal(void) const			{ return -m_xform.col(2).head(3); }

    //float			getFOV(void) const				{ return m_fov; }
    //float			getFOVRad(void) const			{ return m_fov * 3.1415926f / 180.0f; }
    //void			setFOV(float fov)				{ m_fov = fov; }

    float			getFar(void) const				{ return m_far; }
    void			setFar(float f)					{ m_far = f; }

    float			getNear(void) const				{ return m_near; }
    void			setNear(float n)				{ m_near = n; }

    // assumes vertex and index buffers have been bound already
    void			renderShadowMap(int numTriangles, ShaderProgram* pShader, ShadowMapContext& sm, const Vector3f& scene_bb_min, const Vector3f& scene_bb_max);

    // scene bounding box used for focusing shadow map on the actual contents
    Matrix4f		getPosToLightClip(const Vector3f& scene_bb_min, const Vector3f& scene_bb_max) const;

    void			setEnabled(bool e) { m_enabled = e; };
    bool			isEnabled() const { return m_enabled; }

    void            visualize() const; // draw lines with Im3d

    Vector3f			m_color = Vector3f::Ones();

    // OpenGL stuff:
    GLuint			getShadowTextureHandle() const { return m_shadowMapTexture; }
    void			freeShadowMap() { glDeleteTextures(1, &m_shadowMapTexture); m_shadowMapTexture = 0; }
protected:
    static void		renderSceneRaw(int numTriangles, ShaderProgram* prog);

    Vector3f   m_staticPosition = Vector3f::Zero(); // position dictated by scene file, stored separately so that animation doesn't overwrite position data
    Matrix4f	m_xform = Matrix4f::Identity();	// my position and orientation in world space
    //float	m_fov = 45.0f;		// field of view in degrees
    float	m_near = 0.01f;
    float	m_far = 100.0f;

    bool	m_enabled = true;
    GLuint	m_shadowMapTexture = 0;

};
