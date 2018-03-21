#include <mutex>
#include <thread>

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>

#include <jni.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>

ANativeWindow * window = nullptr;
EGLDisplay display = nullptr;
EGLSurface surface = nullptr;
EGLContext context = nullptr;

std::thread render_thread;
std::mutex render_mutex;
bool running = false;

void destroy()
{
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);

    display = nullptr;
    surface = nullptr;
    context = nullptr;
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
#undef CASE_STR
bool init()
{
    if(!window)
        return false;

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
    if (!surface)
    {
        __android_log_print(ANDROID_LOG_ERROR, "INIT", "eglCreateWindowSurface error: %s", eglGetErrorString(eglGetError()));
        destroy();
        return false;
    }

    EGLint context_version[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_version);
    if (!context)
    {
        __android_log_print(ANDROID_LOG_ERROR, "INIT", "eglCreateContext error: %s", eglGetErrorString(eglGetError()));
        destroy();
        return false;
    }

    if (!eglMakeCurrent(display, surface, surface, context))
    {
        __android_log_print(ANDROID_LOG_ERROR, "INIT", "eglMakeCurrent error: %s", eglGetErrorString(eglGetError()));
        destroy();
        return false;
    }

    EGLint width, height;
    if (!eglQuerySurface(display, surface, EGL_WIDTH, &width) || !eglQuerySurface(display, surface, EGL_HEIGHT, &height))
    {
        __android_log_print(ANDROID_LOG_ERROR, "INIT", "eglQuerySurface error: %s", eglGetErrorString(eglGetError()));
        destroy();
        return false;
    }

    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);

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

        if(!display)
        {
            //__android_log_write(ANDROID_LOG_DEBUG, "RENDER_LOOP", "no display");
            continue;
        }

        r += dr; if(r > 1.0f || r < 0.0f) dr = -dr;
        g += dg; if(g > 1.0f || g < 0.0f) dg = -dg;
        b += db; if(b > 1.0f || b < 0.0f) db = -db;

        glClearColor(r, g, b, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
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

JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_setsurface(JNIEnv * env, jobject, jobject surface)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "setsurface");
    if(surface != 0)
    {
        window = ANativeWindow_fromSurface(env, surface);
    }
    else
    {
        ANativeWindow_release(window);
    }
}
}
