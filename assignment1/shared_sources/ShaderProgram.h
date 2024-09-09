#pragma once

#include <string>

using namespace std;

#define FW_GL_SHADER_SOURCE(CODE) #CODE

class ShaderProgram
{
public:
	ShaderProgram(const string& vertexSource, const string& fragmentSource);

	ShaderProgram(const string& vertexSource,
				  GLenum geomInputType, GLenum geomOutputType, int geomVerticesOut, const string& geometrySource,
				  const string& fragmentSource);

	~ShaderProgram(void);

	GLuint          getHandle(void) const { return m_glProgram; }
	GLint           getAttribLoc(const string& name) const;
	GLint           getUniformLoc(const string& name) const;

	void            use(void);

	static GLuint   createGLShader(GLenum type, const string& typeStr, const string& source);
	static void     linkGLProgram(GLuint prog);

private:
	void            init(const string& vertexSource,
						 GLenum geomInputType, GLenum geomOutputType, int geomVerticesOut, const string& geometrySource,
						 const string& fragmentSource);

private:
	ShaderProgram(const ShaderProgram&) {}	// forbidden
	ShaderProgram& operator= (const ShaderProgram&) {}		// forbidden

private:
	GLuint          m_glVertexShader;
	GLuint          m_glGeometryShader;
	GLuint          m_glFragmentShader;
	GLuint          m_glProgram;
};
