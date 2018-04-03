#ifndef INC_2050_OPENGL_HPP
#define INC_2050_OPENGL_HPP

#include <string>
#include <unordered_map>
#include <vector>

#include <android/log.h>

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

    Shader_prog(const std::vector<std::pair<std::string, GLenum>> & sources,
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
        Shader_obj(const std::string & src, GLenum type);
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

class Quad
{
private:
    GL_buffer vbo{GL_ARRAY_BUFFER};
    std::size_t num_indexes;
    std::vector<std::size_t> offsets;

public:
    Quad();
    void draw() const;

};

#endif //INC_2050_OPENGL_HPP