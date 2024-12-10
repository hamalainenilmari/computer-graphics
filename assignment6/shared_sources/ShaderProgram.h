/*
 *  Copyright (c) 2009-2011, NVIDIA Corporation
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *      * Neither the name of NVIDIA Corporation nor the
 *        names of its contributors may be used to endorse or promote products
 *        derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Adapted for Aalto CS-C3100 Computer Graphics from source code with the copyright above.

#pragma once

#include <string>

using namespace std;
using namespace Eigen;      // enables writing "Vector3f" instead of "Eigen::Vector3f", etc.

#define FW_GL_SHADER_SOURCE(CODE) #CODE

class ShaderProgram
{
public:
    struct ShaderCompilationException
    {
        ShaderCompilationException(string msg) : msg_(msg) { }
        string msg_;
    };

	ShaderProgram(const string& vertexSource, const string& fragmentSource);

	ShaderProgram(const string& vertexSource,
				  GLenum geomInputType, GLenum geomOutputType, int geomVerticesOut, const string& geometrySource,
				  const string& fragmentSource);

	~ShaderProgram(void);

	GLuint          getHandle(void) const { return m_glProgram; }
	GLint           getAttribLoc(const string& name) const;
	GLint           getUniformLoc(const string& name) const;

    void            setUniform(int loc, int v) { if (loc >= 0) glUniform1i(loc, v); }
    void            setUniform(int loc, float v) { if (loc >= 0) glUniform1f(loc, v); }
    void            setUniform(int loc, double v) { if (loc >= 0) glUniform1d(loc, v); }
    void            setUniform(int loc, const Vector2f& v) { if (loc >= 0) glUniform2f(loc, v(0), v(1)); }
    void            setUniform(int loc, const Vector3f& v) { if (loc >= 0) glUniform3f(loc, v(0), v(1), v(2)); }
    void            setUniform(int loc, const Vector4f& v) { if (loc >= 0) glUniform4f(loc, v(0), v(1), v(2), v(3)); }
    void            setUniform(int loc, const Matrix2f& v) { if (loc >= 0) glUniformMatrix2fv(loc, 1, false, v.data()); }
    void            setUniform(int loc, const Matrix3f& v) { if (loc >= 0) glUniformMatrix3fv(loc, 1, false, v.data()); }
    void            setUniform(int loc, const Matrix4f& v) { if (loc >= 0) glUniformMatrix4fv(loc, 1, false, v.data()); }
    void            setUniformArray(int loc, int count, int* v) { if (loc >= 0) glUniform1iv(loc, count, v); }
    void            setUniformArray(int loc, int count, float* v) { if (loc >= 0) glUniform1fv(loc, count, v); }
    void            setUniformArray(int loc, int count, double* v) { if (loc >= 0) glUniform1dv(loc, count, v); }
    void            setUniformArray(int loc, int count, const Vector2f* v) { if (loc >= 0) glUniform2fv(loc, count, v->data()); }
    void            setUniformArray(int loc, int count, const Vector3f* v) { if (loc >= 0) glUniform3fv(loc, count, v->data()); }
    void            setUniformArray(int loc, int count, const Vector4f* v) { if (loc >= 0) glUniform4fv(loc, count, v->data()); }
    void            setUniformArray(int loc, int count, const Matrix2f* v) { if (loc >= 0) glUniformMatrix2fv(loc, count, false, v->data()); }
    void            setUniformArray(int loc, int count, const Matrix3f* v) { if (loc >= 0) glUniformMatrix3fv(loc, count, false, v->data()); }
    void            setUniformArray(int loc, int count, const Matrix4f* v) { if (loc >= 0) glUniformMatrix4fv(loc, count, false, v->data()); }

    void            setUniform(const string& name, int v) { setUniform(getUniformLoc(name), v); }
    void            setUniform(const string& name, float v) { setUniform(getUniformLoc(name), v); }
    void            setUniform(const string& name, double v) { setUniform(getUniformLoc(name), v); }
    void            setUniform(const string& name, const Vector2f& v) { setUniform(getUniformLoc(name), v); }
    void            setUniform(const string& name, const Vector3f& v) { setUniform(getUniformLoc(name), v); }
    void            setUniform(const string& name, const Vector4f& v) { setUniform(getUniformLoc(name), v); }
    void            setUniform(const string& name, const Matrix2f& v) { setUniform(getUniformLoc(name), v); }
    void            setUniform(const string& name, const Matrix3f& v) { setUniform(getUniformLoc(name), v); }
    void            setUniform(const string& name, const Matrix4f& v) { setUniform(getUniformLoc(name), v); }
    void            setUniformArray(const string& name, int count, int* v) { setUniformArray(getUniformLoc(name), count, v); }
    void            setUniformArray(const string& name, int count, float* v) { setUniformArray(getUniformLoc(name), count, v); }
    void            setUniformArray(const string& name, int count, double* v) { setUniformArray(getUniformLoc(name), count, v); }
    void            setUniformArray(const string& name, int count, const Vector2f* v) { setUniformArray(getUniformLoc(name), count, v); }
    void            setUniformArray(const string& name, int count, const Vector3f* v) { setUniformArray(getUniformLoc(name), count, v); }
    void            setUniformArray(const string& name, int count, const Vector4f* v) { setUniformArray(getUniformLoc(name), count, v); }
    void            setUniformArray(const string& name, int count, const Matrix2f* v) { setUniformArray(getUniformLoc(name), count, v); }
    void            setUniformArray(const string& name, int count, const Matrix3f* v) { setUniformArray(getUniformLoc(name), count, v); }
    void            setUniformArray(const string& name, int count, const Matrix4f* v) { setUniformArray(getUniformLoc(name), count, v); }

	void            use(void);

	static GLuint   createGLShader(GLenum type, const string& typeStr, const string& source);
	static void     linkGLProgram(GLuint prog);

private:
	void            init(const string& vertexSource,
						 GLenum geomInputType, GLenum geomOutputType, int geomVerticesOut, const string& geometrySource,
						 const string& fragmentSource);

private:
	ShaderProgram(const ShaderProgram&) = delete; // forbidden
	ShaderProgram& operator= (const ShaderProgram&) = delete; // forbidden

private:
	GLuint          m_glVertexShader;
	GLuint          m_glGeometryShader;
	GLuint          m_glFragmentShader;
	GLuint          m_glProgram;
};
