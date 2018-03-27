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
                const std::vector<std::pair<std::string, GLuint>> & attribs);
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

// NOTE: only supports monochrome textures
class Texture2D
{
private:
    GLuint id;
public:
    Texture2D(const void * data, std::size_t w, std::size_t h);
    ~Texture2D();

    Texture2D(const Texture2D &) = delete;
    Texture2D & operator=(const Texture2D &) = delete;
    Texture2D(Texture2D && other);
    Texture2D & operator=(Texture2D && other);

    GLuint get_id() const;
    void bind() const;

    static Texture2D gen_circle_tex(const std::size_t size);
    static Texture2D gen_1pix_tex();
};

class Quad
{
private:
    GL_buffer vbo{GL_ARRAY_BUFFER};
    const GLsizei num_indexes = 4;
public:
    Quad();
    void draw() const;

};

template <std::size_t s, GLenum t, typename T>
struct Buffer_data
{
    using value_type = T;

    static const std::size_t attrib_size = s;
    static const GLenum gl_type = t;

    const void * p;
    const void * data() const { return p; }

    std::size_t bytes;
    std::size_t size() const { return bytes; }

    template<typename Vec>
    Buffer_data(const Vec & vec): p(std::data(vec)), bytes(std::size(vec) * sizeof(typename Vec::value_type)) {}
};

template<typename Vec, typename ... Vecs>
void GL_bind_sub_data(std::size_t offset, GLuint attrib, GLenum target, const Vec & vec, const Vecs & ... vecs)
{
    glBufferSubData(target, offset, std::size(vec), std::data(vec));
    glVertexAttribPointer(attrib, Vec::attrib_size, Vec::gl_type, GL_FALSE, 0, reinterpret_cast<const GLvoid *>(offset));
    glEnableVertexAttribArray(attrib);

    if constexpr(sizeof ...(vecs) > 0)
    {
        GL_bind_sub_data(offset += std::size(vec), attrib + 1, target, vecs ...);
    }
}

template<typename ... Vecs>
void GL_bind_data(GLenum target, const Vecs & ... vecs)
{
    glBufferData(target, (0 + ... + (std::size(vecs) * sizeof(typename Vecs::value_type))), NULL, GL_STATIC_DRAW);
    GL_bind_sub_data(0, 0, target, vecs ...);
}


#endif //INC_2050_OPENGL_HPP