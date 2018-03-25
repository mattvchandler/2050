#include <memory>

#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <android/log.h>

#include <jni.h>

#include "engine.hpp"

/*
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
            std::vector<char> log(static_cast<std::size_t>(log_length + 1), '\0');
            glGetProgramInfoLog(id, log_length, NULL, log.data());

            glDeleteProgram(id);
            id = 0;

            __android_log_print(ANDROID_LOG_ERROR, "Shader_prog::Shader_proj", "Error linking shader program:\n %s", log.data());
            throw std::system_error(link_status, std::system_category(), "Error linking shader program:\n" + std::string(log.data()));
        }

        // get uniforms
        GLint num_uniforms;
        GLint max_buff_size;
        glGetProgramiv(id, GL_ACTIVE_UNIFORMS, &num_uniforms);
        glGetProgramiv(id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_buff_size);

        std::vector<char> uniform(static_cast<std::size_t>(max_buff_size + 1), '\0');

        for(GLuint i = 0; i < static_cast<GLuint>(num_uniforms); ++i)
        {
            GLint size; GLenum type;
            glGetActiveUniform(id, i, static_cast<GLint>(uniform.size()), NULL, &size, &type, uniform.data());

            GLint loc = glGetUniformLocation(id, uniform.data());
            if(loc != -1)
                uniforms[uniform.data()] = loc;
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
                std::vector<char> log(static_cast<std::size_t>(log_length), '\0');
                glGetShaderInfoLog(id, log_length, NULL, log.data());

                glDeleteShader(id);
                id = 0;

                __android_log_print(ANDROID_LOG_ERROR, "Shader_prog::Shader_obj::Shader_obj", "Error compiling shader:\n %s", log.data());
                throw std::system_error(compile_status, std::system_category(), "Error compiling shader:\n" + std::string(log.data()));
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
 */

std::unique_ptr<Engine> engine;

extern "C"
{
JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_start(JNIEnv * env, jobject, jobject asset_mgr, jstring jpath)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "start");
    try
    {
        engine = std::make_unique<Engine>();
        engine->set_asset_manager(AAssetManager_fromJava(env, asset_mgr));
        engine->set_data_path(env->GetStringUTFChars(jpath, NULL));
    }
    catch(std::exception &e)
    {
        __android_log_print(ANDROID_LOG_ERROR, "JNI", "Could not init engine: %s", e.what());
    }
}

JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_resume(JNIEnv, jobject)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "resume");

    if(engine)
        engine->start();
    else
        __android_log_write(ANDROID_LOG_ERROR, "JNI", "resume called before engine init");
}
JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_pause(JNIEnv, jobject)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "pause");

    if(engine)
        engine->stop();
    else
        __android_log_write(ANDROID_LOG_ERROR, "JNI", "pause called before engine init");
}

JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_stop(JNIEnv, jobject)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "stop");
    engine.reset();
}

JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_setSurface(JNIEnv * env, jobject, jobject surface)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "setSurface");
    if(!engine)
    {
        __android_log_write(ANDROID_LOG_ERROR, "JNI", "setSurface called before engine init");
        return;
    }

    if(surface)
        engine->set_window(ANativeWindow_fromSurface(env, surface));
    else
        engine->set_window(nullptr);
}

JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_fling(JNIEnv, jobject, jfloat x, jfloat y)
{
    __android_log_print(ANDROID_LOG_DEBUG, "JNI", "fling [%f, %f]", (float)x, (float)y);

    if(engine)
        engine->fling(x, y);
    else
        __android_log_write(ANDROID_LOG_ERROR, "JNI", "fling called before engine init");
}
}
