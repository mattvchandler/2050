/// @file
/// @brief Shared font data

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

#include "font_impl.hpp"

#include <system_error>

// include shader source strings (this file is assembled from shader files by CMake)
#include "shaders.inl"

namespace textogl
{
    Font_sys::Impl::Font_common::Font_common()
    {
        FT_Error err = FT_Init_FreeType(&ft_lib);
        if(err != FT_Err_Ok)
        {
            throw std::system_error(err, std::system_category(), "Error loading freetype library");
        }

        GLuint vert = glCreateShader(GL_VERTEX_SHADER);
        GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);

        glShaderSource(vert, 1, &vert_shader_src, NULL);
        glShaderSource(frag, 1, &frag_shader_src, NULL);

        for(auto i : {std::make_pair(vert, "vertex"), std::make_pair(frag, "fragement")})
        {
            glCompileShader(i.first);

            GLint compile_status;
            glGetShaderiv(i.first, GL_COMPILE_STATUS, &compile_status);

            if(compile_status != GL_TRUE)
            {
                GLint log_length;
                glGetShaderiv(i.first, GL_INFO_LOG_LENGTH, &log_length);
                std::vector<char> log(log_length + 1);
                log.back() = '\0';
                glGetShaderInfoLog(i.first, log_length, NULL, log.data());

                glDeleteShader(vert);
                glDeleteShader(frag);
                FT_Done_FreeType(ft_lib);

                throw std::system_error(compile_status, std::system_category(), std::string("Error compiling ") + i.second + " shader: \n" +
                        std::string(log.data()));
            }
        }

        // create program and attatch new shaders to it
        prog = glCreateProgram();
        glAttachShader(prog, vert);
        glAttachShader(prog, frag);

        // set attr locations
        glBindAttribLocation(prog, 0, "vert_pos");
        glBindAttribLocation(prog, 1, "vert_tex_coords");

        glLinkProgram(prog);

        // detatch and delete shaders
        glDetachShader(prog, vert);
        glDetachShader(prog, frag);
        glDeleteShader(vert);
        glDeleteShader(frag);

        // check for link errors
        GLint link_status;
        glGetProgramiv(prog, GL_LINK_STATUS, &link_status);
        if(link_status != GL_TRUE)
        {
            GLint log_length;
            glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_length);
            std::vector<char> log(log_length + 1);
            log.back() = '\0';
            glGetProgramInfoLog(prog, log_length, NULL, log.data());

            FT_Done_FreeType(ft_lib);
            glDeleteProgram(prog);

            throw std::system_error(link_status, std::system_category(), std::string("Error linking shader program:\n") +
                    std::string(log.data()));
        }

        // get uniform locations
        GLint num_uniforms = 0;
        GLint max_buff_size = 0;
        glGetProgramiv(prog, GL_ACTIVE_UNIFORMS, &num_uniforms);
        glGetProgramiv(prog, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_buff_size);

        std::vector<GLchar> uniform(max_buff_size, '\0');

        for(GLuint i = 0; i < static_cast<GLuint>(num_uniforms); ++i)
        {
            GLint size; GLenum type;
            glGetActiveUniform(prog, i, static_cast<GLsizei>(uniform.size()), NULL, &size, &type, uniform.data());

            GLint loc = glGetUniformLocation(prog, uniform.data());
            if(loc != -1)
                uniform_locations[uniform.data()] = loc;
        }
    }

    Font_sys::Impl::Font_common::~Font_common()
    {
        FT_Done_FreeType(ft_lib);
        glDeleteProgram(prog);
    }

    unsigned int Font_sys::Impl::_common_ref_cnt = 0;
    std::unique_ptr<Font_sys::Impl::Font_common> Font_sys::Impl::_common_data;
}
