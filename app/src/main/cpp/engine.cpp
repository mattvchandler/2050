#include "engine.hpp"
#include "world.hpp"

#include <chrono>

#include <android/log.h>

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

void Engine::set_window(ANativeWindow * window) noexcept
{
    if(window)
    {
        win = window;
        if(!init_display())
        {
            __android_log_write(ANDROID_LOG_ERROR, "Engine::start", "Could not create display");
            return;
        }
    }
    else
    {
        ANativeWindow_release(win);
        win = nullptr;
    }
}
void Engine::set_asset_manager(AAssetManager * manager) noexcept
{
    asset_manager = manager;
}
void Engine::set_data_path(const std::string & path) noexcept
{
    data_path = path;
}

bool Engine::init_display() noexcept
{
    __android_log_write(ANDROID_LOG_DEBUG, "Engine::init_display", "init start");
    if(!win)
    {
        __android_log_write(ANDROID_LOG_WARN, "Engine::init_display", "init exited b/c no window");
        return false;
    }

    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if(display == EGL_NO_DISPLAY)
    {
        __android_log_print(ANDROID_LOG_ERROR, "Engine::init_display", "eglGetDisplay error: %s", eglGetErrorString(eglGetError()));
        return false;
    }

    if(!eglInitialize(display, NULL, NULL))
    {
        __android_log_print(ANDROID_LOG_ERROR, "Engine::init_display", "eglGetInitialize error: %s", eglGetErrorString(eglGetError()));
        return false;
    }

    EGLint attribs[] =
            {
                    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
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
        __android_log_print(ANDROID_LOG_ERROR, "Engine::init_display", "eglChooseConfig error: %s", eglGetErrorString(eglGetError()));
        destroy_display();
        return false;
    }

    EGLint native_visual_id;
    if(!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &native_visual_id))
    {
        __android_log_print(ANDROID_LOG_ERROR, "Engine::init_display", "eglConfigAttrib error: %s", eglGetErrorString(eglGetError()));
        destroy_display();
        return false;
    }

    ANativeWindow_setBuffersGeometry(win, 0, 0, native_visual_id);

    surface = eglCreateWindowSurface(display, config, win, NULL);
    if(surface == EGL_NO_SURFACE)
    {
        __android_log_print(ANDROID_LOG_ERROR, "Engine::init_display", "eglCreateWindowSurface error: %s", eglGetErrorString(eglGetError()));
        destroy_display();
        return false;
    }

    EGLint context_version[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_version);
    if(context == EGL_NO_CONTEXT)
    {
        __android_log_print(ANDROID_LOG_ERROR, "Engine::init_display", "eglCreateContext error: %s", eglGetErrorString(eglGetError()));
        destroy_display();
        return false;
    }

    if(!eglMakeCurrent(display, surface, surface, context))
    {
        __android_log_print(ANDROID_LOG_ERROR, "Engine::init_display", "eglMakeCurrent error: %s", eglGetErrorString(eglGetError()));
        destroy_display();
        return false;
    }

    if(!eglQuerySurface(display, surface, EGL_WIDTH, &width) || !eglQuerySurface(display, surface, EGL_HEIGHT, &height))
    {
        __android_log_print(ANDROID_LOG_ERROR, "Engine::init_display", "eglQuerySurface error: %s", eglGetErrorString(eglGetError()));
        destroy_display();
        return false;
    }

    __android_log_write(ANDROID_LOG_DEBUG, "Engine::init_display", "EGL set up.");

    return true;
}
void Engine::destroy_display() noexcept
{
    __android_log_write(ANDROID_LOG_DEBUG, "Engine::destroy_display", "Destroying display");

    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);

    display = EGL_NO_DISPLAY;
    surface = EGL_NO_SURFACE;
    context = EGL_NO_CONTEXT;
}

void Engine::start() noexcept
{
    if(running)
        return;

    try
    {
        world = std::make_unique<World>();
    }
    catch(...) // TODO: what exceptions can be thrown
    {
        __android_log_write(ANDROID_LOG_ERROR, "Engine::start", "Could not create world");
    }


    running = true;

    render_thread  = std::thread([this](){ render_loop(); });
    physics_thread = std::thread([this](){ physics_loop(); });
}
void Engine::stop() noexcept
{
    if(!running)
        return;

    running = false;

    try
    {
        render_thread.join();
    }
    catch(std::system_error &e)
    {
        __android_log_print(ANDROID_LOG_ERROR, "Engine::stop", "Error joining render_thread: %s", e.what());
    }

    try
    {
        physics_thread.join();
    }
    catch(std::system_error &e)
    {
        __android_log_print(ANDROID_LOG_ERROR, "Engine::stop", "Error joining physics_thread: %s", e.what());
    }

    world.reset();

    __android_log_write(ANDROID_LOG_DEBUG, "Engine::stop", "Engine stopped");
}

void Engine::render_loop() noexcept
{
    using namespace std::chrono_literals;

    auto last_iteration = std::chrono::system_clock::now();
    while(running)
    {
        if(!win)
            continue;

        std::this_thread::sleep_for(1ms);
        auto dt = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::system_clock::now() - last_iteration).count();

        last_iteration = std::chrono::system_clock::now();
    }
}

void Engine::physics_loop() noexcept
{
    using namespace std::chrono_literals;
    auto last_iteration = std::chrono::system_clock::now();
    while(running)
    {
        if(!win)
            continue;

        std::this_thread::sleep_for(1ms);
        auto dt = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::system_clock::now() - last_iteration).count();

        last_iteration = std::chrono::system_clock::now();
    }
}

void Engine::fling(float x, float y)
{

}
