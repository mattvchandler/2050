#include "engine.hpp"
#include "world.hpp"

#include <chrono>
using namespace std::chrono_literals;

#include <android/log.h>
#include <GLES2/gl2.h>

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

void Engine::destroy_egl()
{
    __android_log_write(ANDROID_LOG_DEBUG, "Engine::destroy_egl", "Destroying display");

    world.destroy();

    if(display != EGL_NO_DISPLAY)
    {
        if(eglGetCurrentContext() != EGL_NO_CONTEXT)
            eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        __android_log_write(ANDROID_LOG_DEBUG, "Engine::destroy_egl", "unbound");
        if(context != EGL_NO_CONTEXT)
            eglDestroyContext(display, context);
        __android_log_write(ANDROID_LOG_DEBUG, "Engine::destroy_egl", "context");
        if(surface != EGL_NO_SURFACE)
            eglDestroySurface(display, surface);
        __android_log_write(ANDROID_LOG_DEBUG, "Engine::destroy_egl", "surface");

        eglTerminate(display);
        __android_log_write(ANDROID_LOG_DEBUG, "Engine::destroy_egl", "display");
    }

    display = EGL_NO_DISPLAY;
    surface = EGL_NO_SURFACE;
    context = EGL_NO_CONTEXT;
};

bool Engine::init_egl()
{
    std::scoped_lock lock(mutex);
    __android_log_write(ANDROID_LOG_DEBUG, "Engine::init_egl", "begin egl initialization");
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if(display == EGL_NO_DISPLAY)
    {
        __android_log_print(ANDROID_LOG_ERROR, "Engine::init_egl", "eglGetDisplay error: %s", eglGetErrorString(eglGetError()));
        return false;
    }

    if(!eglInitialize(display, NULL, NULL))
    {
        __android_log_print(ANDROID_LOG_ERROR, "Engine::init_egl", "eglGetInitialize error: %s", eglGetErrorString(eglGetError()));
        return false;
    }

    __android_log_write(ANDROID_LOG_DEBUG, "Engine::init_egl", "egl initialized");
    return true;
}
bool Engine::init_surface()
{
    __android_log_write(ANDROID_LOG_DEBUG, "Engine::init_surface", "begin surface creation");
    EGLint native_visual_id;
    if(!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &native_visual_id))
    {
        __android_log_print(ANDROID_LOG_ERROR, "Engine::init_surface", "eglConfigAttrib error: %s", eglGetErrorString(eglGetError()));
        return false;
    }

    ANativeWindow_setBuffersGeometry(win, 0, 0, native_visual_id);

    surface = eglCreateWindowSurface(display, config, win, NULL);
    if(surface == EGL_NO_SURFACE)
    {
        __android_log_print(ANDROID_LOG_ERROR, "Engine::init_surface", "eglCreateWindowSurface error: %s", eglGetErrorString(eglGetError()));
        return false;
    }

    if(!eglQuerySurface(display, surface, EGL_WIDTH, &width) || !eglQuerySurface(display, surface, EGL_HEIGHT, &height))
    {
        __android_log_print(ANDROID_LOG_ERROR, "Engine::init_surface", "eglQuerySurface error: %s", eglGetErrorString(eglGetError()));
        eglDestroySurface(display, surface);
        return false;
    }

    if(height <= 0 || width <= 0)
    {
        __android_log_print(ANDROID_LOG_WARN, "Engine::init_surface", "eglQuerySurface returned invalid size: %d x %d", width, height);
        eglDestroySurface(display, surface);
        return false;
    }

    __android_log_write(ANDROID_LOG_DEBUG, "Engine::init_surface", "surface created");
    return true;
}
bool Engine::init_context()
{
    __android_log_write(ANDROID_LOG_DEBUG, "Engine::init_context", "begin context creation");
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
    EGLint num_configs;
    if(!eglChooseConfig(display, attribs, &config, 1, &num_configs))
    {
        __android_log_print(ANDROID_LOG_ERROR, "Engine::init_context", "eglChooseConfig error: %s", eglGetErrorString(eglGetError()));
        return false;
    }

    EGLint context_version[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_version);
    if(context == EGL_NO_CONTEXT)
    {
        __android_log_print(ANDROID_LOG_ERROR, "Engine::init_context", "eglCreateContext error: %s", eglGetErrorString(eglGetError()));
        return false;
    }

    __android_log_write(ANDROID_LOG_DEBUG, "Engine::init_context", "context created");

    return true;
}

bool Engine::can_render()
{
    if(eglGetCurrentContext() == EGL_NO_CONTEXT)
    {
        if(surface == EGL_NO_SURFACE)
        {
            if(context == EGL_NO_CONTEXT)
            {
                if(!init_context())
                {
                    return false;
                }
            }

            if(!has_surface)
                return false;

            if(!init_surface())
                return false;
        }

        if(!eglMakeCurrent(display, surface, surface, context))
        {
            __android_log_print(ANDROID_LOG_ERROR, "Engine::can_render", "eglMakeCurrent error: %s", eglGetErrorString(eglGetError()));
            return false;
        }

        world.init();
        world.resize(width, height);
    }
    return true;
}

void Engine::render_loop()
{
    __android_log_write(ANDROID_LOG_DEBUG, "Engine::render_loop", "start render loop");
    init_egl();

    const std::chrono::duration<float> target_frametime{ 1.0f / 30.0f};

    while(running)
    {
        mutex.lock();
        auto frame_start_time = std::chrono::steady_clock::now();

        if(!resumed || !can_render())
        {
            mutex.unlock();
            std::this_thread::sleep_until(frame_start_time + target_frametime);
            continue;
        }


        int new_width, new_height;
        if(!eglQuerySurface(display, surface, EGL_WIDTH, &new_width) || !eglQuerySurface(display, surface, EGL_HEIGHT, &new_height))
        {
            __android_log_print(ANDROID_LOG_ERROR, "Engine::render_loop", "eglQuerySurface error: %s", eglGetErrorString(eglGetError()));
        }

        if(new_width <= 0 || new_height <= 0)
        {
            mutex.unlock();
            std::this_thread::sleep_for(target_frametime);
            continue;
        }

        if(new_width != width || new_height != height)
        {
            width = new_width; height = new_height;
            world.resize(width, height);
        }

        if(focused)
        {
            world.render();
        }
        else
        {
            // TODO: pause screen
        }

        if(!eglSwapBuffers(display, surface))
        {
            __android_log_write(ANDROID_LOG_ERROR, "Engine::render_loop", "couldn't swap");
        }
        mutex.unlock();
        std::this_thread::sleep_until(frame_start_time + target_frametime);
    }

    destroy_egl();
    __android_log_write(ANDROID_LOG_DEBUG, "Engine::render_loop", "end render loop");
}

void Engine::physics_loop()
{
    __android_log_write(ANDROID_LOG_DEBUG, "Engine::physics_loop", "start physics loop");

    const std::chrono::duration<float> target_frametime{ 1.0f / 30.0f};
    auto last_frame_time = std::chrono::steady_clock::now() - target_frametime;

    while(running)
    {
        mutex.lock();
        auto frame_start_time = std::chrono::steady_clock::now();
        auto dt = std::chrono::duration_cast<std::chrono::duration<float>>(frame_start_time - last_frame_time).count();
        last_frame_time = frame_start_time;

        if(!resumed)
        {
            mutex.unlock();
            std::this_thread::sleep_until(frame_start_time + target_frametime);
            continue;
        }

        if(focused)
        {
            world.physics_step(dt);
        }

        mutex.unlock();
        std::this_thread::sleep_until(frame_start_time + target_frametime);
    }

    destroy_egl();
    __android_log_write(ANDROID_LOG_DEBUG, "Engine::physics_loop", "end physics loop");
}

void Engine::start() noexcept
{

}
void Engine::resume() noexcept
{
    resumed = true;
    running = true;
    render_thread = std::thread(&Engine::render_loop, this);
    physics_thread = std::thread(&Engine::physics_loop, this);
}
void Engine::pause() noexcept
{
    resumed = false;
    // TODO: pause screen, kick off single render event
    running = false;
    render_thread.join();
    physics_thread.join();
}
void Engine::stop() noexcept
{
    // TODO: destroy EGL and openGL stuff
    // TODO: stop render thread
}
void Engine::set_focus(bool focus) noexcept
{
    focused = focus;
    // TODO: if not focused, pause screen, kick off single render event
}

// TODO: can probably collapse these into 1 function
void Engine::surface_created(ANativeWindow *window) noexcept
{
    has_surface = true;
}
void Engine::surface_destroyed() noexcept
{
    std::scoped_lock lock(mutex);
    has_surface = false;
    ANativeWindow_release(win);
    win = nullptr;
}

void Engine::surface_changed(ANativeWindow *window) noexcept
{
    std::scoped_lock lock(mutex);
    win = window;
}

void Engine::fling(float x, float y) noexcept
{
    std::scoped_lock lock(mutex);
}
