// Copyright 2019 Matthew Chandler

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

#include "opengl.hpp"

#include <system_error>

#include <glm/glm.hpp>

#include "log.hpp"

namespace detail
{
#define CASE_STR(value) case value: return #value;
    const char *glGetErrorString(GLenum error)
    {
        switch(error)
        {
            CASE_STR(GL_NO_ERROR)
            CASE_STR(GL_INVALID_ENUM)
            CASE_STR(GL_INVALID_VALUE)
            CASE_STR(GL_INVALID_OPERATION)
            CASE_STR(GL_INVALID_FRAMEBUFFER_OPERATION)
            CASE_STR(GL_OUT_OF_MEMORY)
            default:
                return "Unknown";
        }
    }
#undef CASE_STR

    void GL_check_error(const std::string & at, const char * file, int line)
    {
        GLenum e = glGetError();
        if(e != GL_NO_ERROR)
        {
            using namespace std::string_literals;
            LOG_ERROR_PRINT("GL_check_error", "Error at %s (%s:%d): %s", at.c_str(), file, line, glGetErrorString(e));
        }
    }
}

Shader_prog::Shader_prog(const std::vector<std::pair<std::string_view, GLenum>> & sources,
            const std::vector<std::string> & attribs)
{
    std::vector<Shader_obj> shaders;
    for(auto &source: sources)
        shaders.emplace_back(source.first, source.second);

    id = glCreateProgram();

    for(auto & s: shaders)
        glAttachShader(id, s.get_id());

    // bind given attributes (must be done before link)
    for(GLuint i = 0; i < std::size(attribs); ++i)
        glBindAttribLocation(id, i, attribs[i].c_str());

    glLinkProgram(id);

    GLint link_status;
    glGetProgramiv(id, GL_LINK_STATUS, &link_status);
    if(link_status != GL_TRUE)
    {
        GLint log_length;
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &log_length);
        std::string log(log_length, '\0');
        glGetProgramInfoLog(id, log_length, nullptr, std::data(log));

        glDeleteProgram(id);
        id = 0;

        LOG_ERROR_PRINT("Shader_prog::Shader_proj", "Error linking shader program:\n %s", log.c_str());
        throw std::system_error(link_status, std::system_category(), "Error linking shader program:\n" + log);
    }

    // get uniforms
    GLint num_uniforms;
    GLint max_buff_size;
    glGetProgramiv(id, GL_ACTIVE_UNIFORMS, &num_uniforms);
    glGetProgramiv(id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_buff_size);

    std::vector<char> uniform(static_cast<std::size_t>(max_buff_size), '\0');

    for(GLuint i = 0; i < static_cast<GLuint>(num_uniforms); ++i)
    {
        GLint size; GLenum type;
        glGetActiveUniform(id, i, static_cast<GLint>(uniform.size()), nullptr, &size, &type, std::data(uniform));

        GLint loc = glGetUniformLocation(id, std::data(uniform));
        if(loc != -1)
            uniforms.emplace(std::string(std::data(uniform)), loc);
    }
}
Shader_prog::~Shader_prog()
{
    if(id)
        glDeleteProgram(id);
}
Shader_prog::Shader_prog(Shader_prog && other) noexcept : uniforms(std::move(other.uniforms)), id(other.id) { other.id = 0; }
Shader_prog & Shader_prog::operator=(Shader_prog && other) noexcept
{
    if(this != &other)
    {
        uniforms = std::move(other.uniforms);
        id = other.id;
        other.id = 0;
    }
    return *this;
}

void Shader_prog::use() const { glUseProgram(id); }
GLint Shader_prog::get_uniform(const std::string & uniform) const
{
    try
    {
        return uniforms.at(uniform);
    }
    catch(std::out_of_range & e)
    {
        LOG_ERROR_PRINT("Shader_prog::get_uniform", "Could not find uniform %s: %s", uniform.c_str(), e.what());
        return 0;
    }
}

Shader_prog::Shader_obj::Shader_obj(const std::string_view & src, GLenum type)
{
    id  = glCreateShader(type);

    auto data = static_cast<const GLchar *>(std::data(src));
    auto length = static_cast<GLint>(std::size(src));
    glShaderSource(id, 1, &data, &length);
    glCompileShader(id);

    GLint compile_status;
    glGetShaderiv(id, GL_COMPILE_STATUS, &compile_status);

    if(compile_status != GL_TRUE)
    {
        GLint log_length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_length);
        std::string log(log_length, '\0');
        glGetShaderInfoLog(id, log_length, nullptr, std::data(log));

        glDeleteShader(id);
        id = 0;

        LOG_ERROR_PRINT("Shader_prog::Shader_obj::Shader_obj", "Error compiling shader:\n %s", log.c_str());
        throw std::system_error(compile_status, std::system_category(), "Error compiling shader:\n" + log);
    }
}
Shader_prog::Shader_obj::~Shader_obj()
{
    if(id)
        glDeleteShader(id);
}
Shader_prog::Shader_obj::Shader_obj(Shader_obj && other) noexcept : id(other.id) { other.id = 0; }
Shader_prog::Shader_obj & Shader_prog::Shader_obj::operator=(Shader_obj && other) noexcept
{
    if(this != &other)
    {
        id = other.id;
        other.id = 0;
    }
    return *this;
}

GLuint Shader_prog::Shader_obj::get_id() const { return id; }

GL_buffer::GL_buffer(GLenum type):type(type) { glGenBuffers(1, &id); }
GL_buffer::~GL_buffer()
{
    if(id)
        glDeleteBuffers(1, &id);
}
GL_buffer::GL_buffer(GL_buffer && other) noexcept : id(other.id) { other.id = 0; }
GL_buffer & GL_buffer::operator=(GL_buffer && other) noexcept
{
    if(this != &other)
    {
        id = other.id;
        other.id = 0;
    }
    return *this;
}

GLuint GL_buffer::get_id() const { return id; }
void GL_buffer::bind() const { glBindBuffer(type, id); }
