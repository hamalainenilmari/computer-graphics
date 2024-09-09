#include "glad/gl.h"
#include "ShaderProgram.h"
#include "Utils.h"
#include <vector>
#include <fmt/core.h>

using namespace std;

//------------------------------------------------------------------------

ShaderProgram::ShaderProgram(const string& vertexSource, const string& fragmentSource)
{
    init(vertexSource, 0, 0, 0, "", fragmentSource);
}

//------------------------------------------------------------------------

ShaderProgram::ShaderProgram(
    const string& vertexSource,
    GLenum geomInputType, GLenum geomOutputType, int geomVerticesOut, const string& geometrySource,
    const string& fragmentSource)
{
    init(vertexSource, geomInputType, geomOutputType, geomVerticesOut, geometrySource, fragmentSource);
}

//------------------------------------------------------------------------

ShaderProgram::~ShaderProgram(void)
{
    glDeleteProgram(m_glProgram);
    glDeleteShader(m_glVertexShader);
    glDeleteShader(m_glGeometryShader);
    glDeleteShader(m_glFragmentShader);
}

//------------------------------------------------------------------------

GLint ShaderProgram::getAttribLoc(const string& name) const
{
    return glGetAttribLocation(m_glProgram, name.c_str());
}

//------------------------------------------------------------------------

GLint ShaderProgram::getUniformLoc(const string& name) const
{
    return glGetUniformLocation(m_glProgram, name.c_str());
}

//------------------------------------------------------------------------

void ShaderProgram::use(void)
{
    glUseProgram(m_glProgram);
}

//------------------------------------------------------------------------

GLuint ShaderProgram::createGLShader(GLenum type, const string& typeStr, const string& source)
{
    GLuint shader = glCreateShader(type);
    const char* sourcePtr = source.c_str();
    int sourceLen = source.size();
    glShaderSource(shader, 1, &sourcePtr, &sourceLen);
    glCompileShader(shader);

    GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (!infoLen)
            fail(fmt::format("glCompileShader({}) failed!", typeStr));

        string info;
        info.reserve(infoLen);
        glGetShaderInfoLog(shader, infoLen, &infoLen, info.data());
        fail(fmt::format("glCompileShader({}) failed!\n\n{}", typeStr, info));
    }

    // checkErrors();
    return shader;
}

//------------------------------------------------------------------------

void ShaderProgram::linkGLProgram(GLuint prog)
{
    glLinkProgram(prog);
    GLint status = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (!status)
    {
        GLint infoLen = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infoLen);
        if (!infoLen)
            fail("glLinkProgram() failed!");

        
        string info;
        info.reserve(infoLen);
        glGetProgramInfoLog(prog, infoLen, &infoLen, info.data());
        fail(fmt::format("glLinkProgram() failed!\n\n{}", info));
    }
    //checkErrors();
}

//------------------------------------------------------------------------

void ShaderProgram::init(
    const string& vertexSource,
    GLenum geomInputType, GLenum geomOutputType, int geomVerticesOut, const string& geometrySource,
    const string& fragmentSource)
{
    //staticInit();
    m_glProgram = glCreateProgram();

    // Setup vertex shader.

    m_glVertexShader = createGLShader(GL_VERTEX_SHADER, "GL_VERTEX_SHADER", vertexSource);
    glAttachShader(m_glProgram, m_glVertexShader);

    // Setup geometry shader (GL_ARB_geometry_shader4).

    if (geometrySource.size() == 0)
        m_glGeometryShader = 0;
    else
    {
        m_glGeometryShader = createGLShader(GL_GEOMETRY_SHADER_ARB, "GL_GEOMETRY_SHADER_ARB", geometrySource);
        glAttachShader(m_glProgram, m_glGeometryShader);

        if(glProgramParameteriARB == 0)
            fail("glProgramParameteriARB() not available!");
        glProgramParameteriARB(m_glProgram, GL_GEOMETRY_INPUT_TYPE_ARB, geomInputType);
        glProgramParameteriARB(m_glProgram, GL_GEOMETRY_OUTPUT_TYPE_ARB, geomOutputType);
        glProgramParameteriARB(m_glProgram, GL_GEOMETRY_VERTICES_OUT_ARB, geomVerticesOut);
    }

    // Setup fragment shader.

    m_glFragmentShader = createGLShader(GL_FRAGMENT_SHADER, "GL_FRAGMENT_SHADER", fragmentSource);
    glAttachShader(m_glProgram, m_glFragmentShader);

    // Link.

    linkGLProgram(m_glProgram);
}
