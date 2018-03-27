#include "opengl.hpp"

#include <system_error>

#include <glm/glm.hpp>

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
            __android_log_print(ANDROID_LOG_ERROR, "GL_check_error", "Error at %s (%s:%d): %s", at.c_str(), file, line, glGetErrorString(e));
        }
    }
}

Shader_prog::Shader_prog(const std::vector<std::pair<std::string, GLenum>> & sources,
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
Shader_prog::~Shader_prog()
{
    if(id)
        glDeleteProgram(id);
}
Shader_prog::Shader_prog(Shader_prog && other): id(other.id) { other.id = 0; };
Shader_prog & Shader_prog::operator=(Shader_prog && other)
{
    if(this != &other)
    {
        id = other.id;
        other.id = 0;
    }
    return *this;
}

void Shader_prog::use() const { glUseProgram(id); }
GLint Shader_prog::get_uniform(const std::string & uniform) const { return uniforms.at(uniform); }

Shader_prog::Shader_obj::Shader_obj(const std::string & src, GLenum type)
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
Shader_prog::Shader_obj::~Shader_obj()
{
    if(id)
        glDeleteShader(id);
}
Shader_prog::Shader_obj::Shader_obj(Shader_obj && other): id(other.id) { other.id = 0; };
Shader_prog::Shader_obj & Shader_prog::Shader_obj::operator=(Shader_obj && other)
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
GL_buffer::GL_buffer(GL_buffer && other): id(other.id) { other.id = 0; };
GL_buffer & GL_buffer::operator=(GL_buffer && other)
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

Texture2D::Texture2D(const void * data, std::size_t w, std::size_t h)
{
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, static_cast<GLsizei>(w), static_cast<GLsizei>(h), GL_ALPHA, GL_FLOAT, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    GL_CHECK_ERROR("texture generation");
}
Texture2D::~Texture2D()
{
    if(id)
        glDeleteTextures(1, &id);
}
Texture2D::Texture2D(Texture2D && other): id(other.id) { other.id = 0; };
Texture2D & Texture2D::operator=(Texture2D && other)
{
    if(this != &other)
    {
        id = other.id;
        other.id = 0;
    }
    return *this;
}
GLuint Texture2D::get_id() const { return id; }
void Texture2D::bind() const
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, id);
}

Texture2D Texture2D::gen_circle_tex(const std::size_t size)
{
    std::vector<float> circle_tex(size * size);
    for(std::size_t i = 0; i < size; ++i)
    {
        for(std::size_t j = 0; j < size; ++j)
        {
            auto coord = glm::vec2{static_cast<float>(i), static_cast<float>(j)} * 2.0f / (static_cast<float>(size) - 1.0f) - glm::vec2{1.0f, 1.0f};
            if(glm::length(coord) < 1.0f)
                circle_tex[i * size + j] = 1.0f;
            else
                circle_tex[i * size + j] = 0.0f;
        }
    }

    return Texture2D(std::data(circle_tex), size, size);
}
Texture2D Texture2D::gen_1pix_tex()
{
    float pixel = 1.0f;

    auto tex = Texture2D(&pixel, 1, 1);
    tex.bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    return tex;
}

Quad::Quad()
{
    const std::vector<glm::vec2> verts =
    {
        {-0.5f, -0.5f},
        {-0.5f,  0.5f},
        { 0.5f, -0.5f},
        { 0.5f,  0.5f}
    };

    const std::vector<glm::vec2> tex_coords =
    {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f}
    };

    assert(std::size(verts) == std::size(tex_coords));

    num_indexes = std::size(verts);
    offsets.push_back(0);

    vbo.bind();
    glBufferData(GL_ARRAY_BUFFER, (std::size(verts) + std::size(tex_coords)) * sizeof(glm::vec2), NULL, GL_STATIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, offsets.back(), std::size(verts) * sizeof(typename decltype(verts)::value_type), std::data(verts));
    offsets.push_back(offsets.back() + std::size(verts) * sizeof(typename decltype(verts)::value_type));
    glBufferSubData(GL_ARRAY_BUFFER, offsets.back(), std::size(tex_coords) * sizeof(typename decltype(tex_coords)::value_type), std::data(tex_coords));

    GL_CHECK_ERROR("gen quad");
}

void Quad::draw() const
{
    vbo.bind();
    for(std::size_t i = 0; i < std::size(offsets); ++i)
    {
        glVertexAttribPointer(static_cast<GLuint>(i), 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<const GLvoid *>(offsets[i]));
        glEnableVertexAttribArray(static_cast<GLuint>(i));
    }

    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(num_indexes));
    GL_CHECK_ERROR("draw quad");
}
