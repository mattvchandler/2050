#include <mutex>
#include <thread>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>

#include <jni.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

class Shader_prog
{
private:
    std::unordered_map<std::string, GLint> uniforms;
    GLuint id;
public:

    Shader_prog(const std::vector<std::pair<std::string, GLenum>> sources)
    {
        std::vector<Shader_obj> shaders;
        for(auto &source: sources)
            shaders.emplace_back(source.first, source.second);

        id = glCreateProgram();

        for(auto & s: shaders)
            glAttachShader(id, s.get_id());

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


ANativeWindow * window = nullptr;
EGLDisplay display = EGL_NO_DISPLAY;
EGLSurface surface = EGL_NO_SURFACE;
EGLContext context = EGL_NO_CONTEXT;

GLuint vao = 0;
GLuint vbo = 0;
std::unique_ptr<Shader_prog> prog;

std::thread render_thread;
std::mutex render_mutex;
bool running = false;

void destroy()
{
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    prog.reset();

    vao = 0;
    vbo = 0;
    prog = 0;

    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);

    display = EGL_NO_DISPLAY;
    surface = EGL_NO_SURFACE;
    context = EGL_NO_CONTEXT;
}
#define CASE_STR( value ) case value: return #value;
const char* eglGetErrorString( EGLint error )
{
    switch( error )
    {
        CASE_STR( EGL_SUCCESS             )
        CASE_STR( EGL_NOT_INITIALIZED     )
        CASE_STR( EGL_BAD_ACCESS          )
        CASE_STR( EGL_BAD_ALLOC           )
        CASE_STR( EGL_BAD_ATTRIBUTE       )
        CASE_STR( EGL_BAD_CONTEXT         )
        CASE_STR( EGL_BAD_CONFIG          )
        CASE_STR( EGL_BAD_CURRENT_SURFACE )
        CASE_STR( EGL_BAD_DISPLAY         )
        CASE_STR( EGL_BAD_SURFACE         )
        CASE_STR( EGL_BAD_MATCH           )
        CASE_STR( EGL_BAD_PARAMETER       )
        CASE_STR( EGL_BAD_NATIVE_PIXMAP   )
        CASE_STR( EGL_BAD_NATIVE_WINDOW   )
        CASE_STR( EGL_CONTEXT_LOST        )
        default: return "Unknown";
    }
}
const char* glGetErrorString(GLenum error )
{
    switch( error )
    {
    CASE_STR(GL_NO_ERROR)
    CASE_STR(GL_INVALID_ENUM)
    CASE_STR(GL_INVALID_VALUE)
    CASE_STR(GL_INVALID_OPERATION)
    CASE_STR(GL_INVALID_FRAMEBUFFER_OPERATION)
    CASE_STR(GL_OUT_OF_MEMORY)
    default: return "Unknown";
    }
}
#undef CASE_STR

void GL_check_error(const char * at)
{
    GLenum e = glGetError();
    if(e != GL_NO_ERROR)
    {
        using namespace std::string_literals;
        __android_log_print(ANDROID_LOG_ERROR, "Error at %s: %s", at, glGetErrorString(e));
        throw std::system_error(e, std::system_category(), "Error at "s + std::string(at) + ":\n"s + glGetErrorString(e));
    }
}

bool init()
{
    __android_log_write(ANDROID_LOG_DEBUG, "INIT", "init start");
    if(!window)
    {
        __android_log_write(ANDROID_LOG_WARN, "INIT", "init exited b/c no window");
        return false;
    }

    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if(display == EGL_NO_DISPLAY)
    {
        __android_log_print(ANDROID_LOG_ERROR, "INIT", "eglGetDisplay error: %s", eglGetErrorString(eglGetError()));
        return false;
    }

    if(!eglInitialize(display, NULL, NULL))
    {
        __android_log_print(ANDROID_LOG_ERROR, "INIT", "eglGetInitialize error: %s", eglGetErrorString(eglGetError()));
        return false;
    }

    EGLint attribs[] =
            {
                    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
                    EGL_RED_SIZE,   8,
                    EGL_GREEN_SIZE, 8,
                    EGL_BLUE_SIZE,  8,
                    EGL_ALPHA_SIZE, 8,
                    EGL_NONE,
            };
    EGLConfig config;
    EGLint num_configs;
    if(!eglChooseConfig(display, attribs, &config, 1, &num_configs))
    {
        __android_log_print(ANDROID_LOG_ERROR, "INIT", "eglChooseConfig error: %s", eglGetErrorString(eglGetError()));
        destroy();
        return false;
    }

    EGLint native_visual_id;
    if(!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &native_visual_id))
    {
        __android_log_print(ANDROID_LOG_ERROR, "INIT", "eglConfigAttrib error: %s", eglGetErrorString(eglGetError()));
        destroy();
        return false;
    }

    ANativeWindow_setBuffersGeometry(window, 0, 0, native_visual_id);

    surface = eglCreateWindowSurface(display, config, window, NULL);
    if(surface == EGL_NO_SURFACE)
    {
        __android_log_print(ANDROID_LOG_ERROR, "INIT", "eglCreateWindowSurface error: %s", eglGetErrorString(eglGetError()));
        destroy();
        return false;
    }

    EGLint context_version[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_version);
    if(context == EGL_NO_CONTEXT)
    {
        __android_log_print(ANDROID_LOG_ERROR, "INIT", "eglCreateContext error: %s", eglGetErrorString(eglGetError()));
        destroy();
        return false;
    }

    if(!eglMakeCurrent(display, surface, surface, context))
    {
        __android_log_print(ANDROID_LOG_ERROR, "INIT", "eglMakeCurrent error: %s", eglGetErrorString(eglGetError()));
        destroy();
        return false;
    }

    EGLint width, height;
    if(!eglQuerySurface(display, surface, EGL_WIDTH, &width) || !eglQuerySurface(display, surface, EGL_HEIGHT, &height))
    {
        __android_log_print(ANDROID_LOG_ERROR, "INIT", "eglQuerySurface error: %s", eglGetErrorString(eglGetError()));
        destroy();
        return false;
    }

    __android_log_write(ANDROID_LOG_DEBUG, "DEBUG", "EGL set up.");

    GLfloat verts [] =
    {
        -0.5f, -0.5f,
        -0.5f,  0.5f,
         0.5f, -0.5f,
         0.5f,  0.5f
    };

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);
    GL_check_error("vao/vbo");

    const char * vertshader =
 R"(#version 300 es
    layout(location = 0) in vec2 vert_pos;
    void main()
    {
        gl_Position = vec4(vert_pos, 0.0, 1.0);
    }
    )";
    const char * fragshader =
 R"(#version 300 es
    precision mediump float;

    out vec4 frag_color;
    void main()
    {
        frag_color = vec4(0.0, 1.0, 0.0, 1.0);
    }
    )";

    prog = std::make_unique<Shader_prog>(std::vector<std::pair<std::string, GLenum>>{std::make_pair(std::string(vertshader), GL_VERTEX_SHADER), std::make_pair(std::string(fragshader), GL_FRAGMENT_SHADER)});

    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    glViewport(0, 0, width, height);

    __android_log_write(ANDROID_LOG_DEBUG, "DEBUG", "init success");
    return true;
}
void render_loop()
{
    __android_log_write(ANDROID_LOG_DEBUG, "RENDER_LOOP", "start render loop");
    bool window_set = false;
    float r = 0.0f, g = 0.0f, b = 0.0f;
    auto dr = 0.001f, dg = 0.005f, db = 0.010f;
    while(true)
    {
        std::lock_guard<std::mutex> lock(render_mutex);

        if(!running)
        {
            destroy();
            break;
        }

        if(!window_set && window)
        {
            __android_log_write(ANDROID_LOG_DEBUG, "RENDER_LOOP", "init!");
            if(!init())
            {
                __android_log_write(ANDROID_LOG_ERROR, "RENDER_LOOP", "Initialization failed!");
                break;
            }
            window_set = true;
        }

        if(display == EGL_NO_DISPLAY)
        {
            //__android_log_write(ANDROID_LOG_DEBUG, "RENDER_LOOP", "no display");
            continue;
        }

        r += dr; if(r > 1.0f || r < 0.0f) dr = -dr;
        g += dg; if(g > 1.0f || g < 0.0f) dg = -dg;
        b += db; if(b > 1.0f || b < 0.0f) db = -db;

        glClearColor(r, g, b, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        prog->use();
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        GL_check_error("draw");

        if(!eglSwapBuffers(display, surface))
        {
            __android_log_write(ANDROID_LOG_ERROR, "RENDER_LOOP", "couldn't swap!");
        }
    }
    __android_log_write(ANDROID_LOG_DEBUG, "RENDER_LOOP", "end render loop");
}


extern "C"
{
JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_start(JNIEnv, jobject)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "start");
}

JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_resume(JNIEnv, jobject)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "resume");
    running = true;
    render_thread = std::thread(render_loop);
}
JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_pause(JNIEnv, jobject)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "pause");
    render_mutex.lock();
    running = false;
    render_mutex.unlock();

    render_thread.join();
}

JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_stop(JNIEnv, jobject)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "stop");
}

JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_setSurface(JNIEnv * env, jobject, jobject surface)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "setSurface");
    if(surface)
    {
        window = ANativeWindow_fromSurface(env, surface);
    }
    else
    {
        ANativeWindow_release(window);
        window = nullptr;
    }
}

JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_changeGravity(JNIEnv, jobject, jfloat x, jfloat y)
{
    __android_log_print(ANDROID_LOG_DEBUG, "JNI", "changeGravity [%f, %f]", (float)x, (float)y);
}
}
