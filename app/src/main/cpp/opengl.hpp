#ifndef INC_2050_OPENGL_HPP
#define INC_2050_OPENGL_HPP

#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

#include <android/log.h>

#include <GLES2/gl2.h>

#include "opengl.hpp"

class Shader_prog
{
private:
    std::unordered_map<std::string, GLint> uniforms;
    GLuint id;
public:

    Shader_prog(const std::vector<std::pair<std::string, GLenum>> & sources,
                const std::vector<std::pair<std::string, GLuint>> & attribs)
    {
        std::vector<Shader_obj> shaders;
        for(auto &source: sources)
            shaders.emplace_back(source.first, source.second);

        id = glCreateProgram();

        for(auto & s: shaders)
            glAttachShader(id, s.get_id());

        // bind given attributes (must be done before link)
        for(auto & attr: attribs)
            glBindAttribLocation(id, attr.second, attr.first.c_str());

        glLinkProgram(id);

        GLint link_status;
        glGetProgramiv(id, GL_LINK_STATUS, &link_status);
        if(link_status != GL_TRUE)
        {
            GLint log_length;
            glGetProgramiv(id, GL_INFO_LOG_LENGTH, &log_length);
            std::string log(log_length, '\0');
            glGetProgramInfoLog(id, log_length, NULL, std::data(log));

            glDeleteProgram(id);
            id = 0;

            __android_log_print(ANDROID_LOG_ERROR, "Shader_prog::Shader_proj", "Error linking shader program:\n %s", log.c_str());
            throw std::system_error(link_status, std::system_category(), "Error linking shader program:\n" + log);
        }

        // get uniforms
        GLint num_uniforms;
        GLint max_buff_size;
        glGetProgramiv(id, GL_ACTIVE_UNIFORMS, &num_uniforms);
        glGetProgramiv(id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_buff_size);

        std::string uniform(max_buff_size, '\0');

        for(GLuint i = 0; i < static_cast<GLuint>(num_uniforms); ++i)
        {
            GLint size; GLenum type;
            glGetActiveUniform(id, i, static_cast<GLint>(uniform.size()), NULL, &size, &type, uniform.data());

            GLint loc = glGetUniformLocation(id, uniform.c_str());
            if(loc != -1)
                uniforms[uniform.c_str()] = loc;
        }
    }
    ~Shader_prog()
    {
        if(id)
            glDeleteProgram(id);
    }
    Shader_prog(const Shader_prog &) = delete;
    Shader_prog & operator=(const Shader_prog &) = delete;
    Shader_prog(Shader_prog && other): id(other.id) { other.id = 0; };
    Shader_prog & operator=(Shader_prog && other)
    {
        if(this != &other)
        {
            id = other.id;
            other.id = 0;
        }
        return *this;
    }

    void use() const { glUseProgram(id); }
    GLint get_uniform(const std::string & uniform) const { return uniforms.at(uniform); }

    class Shader_obj
    {
    private:
        GLuint id;
    public:
        Shader_obj(const std::string & src, GLenum type)
        {
            id  = glCreateShader(type);

            const char * data = src.c_str();
            glShaderSource(id, 1, &data, NULL);
            glCompileShader(id);

            GLint compile_status;
            glGetShaderiv(id, GL_COMPILE_STATUS, &compile_status);

            if(compile_status != GL_TRUE)
            {
                GLint log_length;
                glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_length);
                std::string log(log_length, '\0');
                glGetShaderInfoLog(id, log_length, NULL, std::data(log));

                glDeleteShader(id);
                id = 0;

                __android_log_print(ANDROID_LOG_ERROR, "Shader_prog::Shader_obj::Shader_obj", "Error compiling shader:\n %s", log.c_str());
                throw std::system_error(compile_status, std::system_category(), "Error compiling shader:\n" + log);
            }
        }
        ~Shader_obj()
        {
            if(id)
                glDeleteShader(id);
        }
        Shader_obj(const Shader_obj &) = delete;
        Shader_obj & operator=(const Shader_obj &) = delete;
        Shader_obj(Shader_obj && other): id(other.id) { other.id = 0; };
        Shader_obj & operator=(Shader_obj && other)
        {
            if(this != &other)
            {
                id = other.id;
                other.id = 0;
            }
            return *this;
        }

        GLuint get_id() const { return id; }
    };
};


#endif //INC_2050_OPENGL_HPP