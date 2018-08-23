// Copyright 2018 Matthew Chandler

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef INC_2050_OPENGL_HPP
#define INC_2050_OPENGL_HPP

#include <string>
#include <unordered_map>
#include <vector>

#include <GLES2/gl2.h>

namespace detail
{
    const char *glGetErrorString(GLenum error);
    void GL_check_error(const std::string & at, const char * file, int line);
}
#define GL_CHECK_ERROR(at) do { detail::GL_check_error(at, __FILE__, __LINE__); } while(false);

class Shader_prog
{
private:
    std::unordered_map<std::string, GLint> uniforms;
    GLuint id;
public:

    Shader_prog(const std::vector<std::pair<std::string_view, GLenum>> & sources,
                const std::vector<std::string> & attribs);
    ~Shader_prog();

    Shader_prog(const Shader_prog &) = delete;
    Shader_prog & operator=(const Shader_prog &) = delete;
    Shader_prog(Shader_prog && other);
    Shader_prog & operator=(Shader_prog && other);

    void use() const;
    GLint get_uniform(const std::string & uniform) const;

    class Shader_obj
    {
    private:
        GLuint id;
    public:
        Shader_obj(const std::string_view & src, GLenum type);
        ~Shader_obj();
        Shader_obj(const Shader_obj &) = delete;
        Shader_obj & operator=(const Shader_obj &) = delete;
        Shader_obj(Shader_obj && other);
        Shader_obj & operator=(Shader_obj && other);
        GLuint get_id() const;
    };
};

class GL_buffer
{
private:
    GLuint id;
    GLenum type;
public:
    GL_buffer(GLenum type);
    ~GL_buffer();
    GL_buffer(const GL_buffer &) = delete;
    GL_buffer & operator=(const GL_buffer &) = delete;
    GL_buffer(GL_buffer && other);
    GL_buffer & operator=(GL_buffer && other);

    GLuint get_id() const;
    void bind() const;
};

#endif //INC_2050_OPENGL_HPP
