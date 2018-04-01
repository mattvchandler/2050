#include "engine.hpp"
#include "world.hpp"

#include <chrono>
#include <fstream>


#include <android/log.h>
#include <GLES2/gl2.h>

using namespace std::chrono_literals;

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

    if(!has_surface)
        return false;

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
    if(display == EGL_NO_DISPLAY)
    {
        __android_log_write(ANDROID_LOG_DEBUG, "Engine::can_render", "can't render: no display");
        return false;
    }

    if(eglGetCurrentContext() == EGL_NO_CONTEXT)
    {
        if(surface == EGL_NO_SURFACE)
        {
            if(context == EGL_NO_CONTEXT)
            {
                if(!init_context())
                {
                    __android_log_write(ANDROID_LOG_DEBUG, "Engine::can_render", "can't render: couldn't init context");
                    return false;
                }
            }

            if(!has_surface)
            {
                __android_log_write(ANDROID_LOG_DEBUG, "Engine::can_render", "can't render: no surface");
                return false;
            }

            if(!init_surface())
            {
                __android_log_write(ANDROID_LOG_DEBUG, "Engine::can_render", "can't render: couldn't init surface");
                return false;
            }
        }

        if(!eglMakeCurrent(display, surface, surface, context))
        {
            __android_log_print(ANDROID_LOG_ERROR, "Engine::can_render", "eglMakeCurrent error: %s", eglGetErrorString(eglGetError()));
            return false;
        }

        __android_log_write(ANDROID_LOG_DEBUG, "Engine::can_render", "set up to render");
        world.init();
        world.resize(width, height);
    }

    return true;
}

void Engine::render_loop()
{
    __android_log_write(ANDROID_LOG_DEBUG, "Engine::render_loop", "start render loop");

    mutex.lock();
    init_egl();
    mutex.unlock();

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

        world.render();

        if(!eglSwapBuffers(display, surface))
        {
            __android_log_write(ANDROID_LOG_ERROR, "Engine::render_loop", "couldn't swap");
        }
        mutex.unlock();
        std::this_thread::sleep_until(frame_start_time + target_frametime);
    }

    mutex.lock();

    world.pause();
    if(can_render())
    {
        world.render();

        if(!eglSwapBuffers(display, surface))
        {
            __android_log_write(ANDROID_LOG_ERROR, "Engine::render_loop", "couldn't swap");
        }
    }
    mutex.unlock();

    world.destroy();
    destroy_egl();

    __android_log_write(ANDROID_LOG_DEBUG, "Engine::render_loop", "end render loop");
}

void Engine::physics_loop()
{
    __android_log_write(ANDROID_LOG_DEBUG, "Engine::physics_loop", "start physics loop");

    const std::chrono::duration<float> target_frametime{10ms};
    auto last_frame_time = std::chrono::steady_clock::now() - target_frametime;

    while(running)
    {
        mutex.lock();
        auto frame_start_time = std::chrono::steady_clock::now();
        auto dt = std::chrono::duration_cast<std::chrono::duration<float>>(frame_start_time - last_frame_time).count();
        dt = std::min(dt, target_frametime.count() * 1.5f);
        last_frame_time = frame_start_time;

        if(resumed && focused)
            world.physics_step(dt);

        mutex.unlock();
        std::this_thread::sleep_until(frame_start_time + target_frametime);
    }

    __android_log_write(ANDROID_LOG_DEBUG, "Engine::physics_loop", "end physics loop");
}

Engine::Engine(AAssetManager * asset_manager, const std::string & data_path): data_path(data_path), world(asset_manager)
{
    std::ifstream savefile(data_path + "/save.json");
    if(savefile)
    {
        nlohmann::json data;
        savefile>>data;
        world.deserialize(data);
    }
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
    // TODO: save openGL context
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
    __android_log_write(ANDROID_LOG_DEBUG, "Engine::stop", "stop");

    std::ofstream savefile(data_path + "/save.json");
    savefile<<world.serialize();
    __android_log_write(ANDROID_LOG_DEBUG, "Engine::stop", "saved data");
}
void Engine::set_focus(bool focus) noexcept
{
    __android_log_print(ANDROID_LOG_DEBUG, "Engine::set_focus", "focused: %s", focus ? "true" : "false");
    focused = focus;
    if(!focused)
    {
        std::scoped_lock lock(mutex);
        world.pause();
    }
}

// TODO: can probably collapse these into 1 function
void Engine::surface_created(ANativeWindow *window) noexcept
{
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
    __android_log_print(ANDROID_LOG_DEBUG, "Engine::surface_changed", "surfaceChanged, with size: %d x %d", ANativeWindow_getWidth(window), ANativeWindow_getHeight(window));
    std::scoped_lock lock(mutex);
    has_surface = true;
    win = window;
}

void Engine::fling(float x, float y) noexcept
{
    std::scoped_lock lock(mutex);
    world.fling(x, y);
}

void Engine::tap(float x, float y) noexcept
{
    std::scoped_lock lock(mutex);
    world.tap(x, y);
}

void Engine::new_game() noexcept
{
    std::scoped_lock lock(mutex);
    world.new_game();
}

void Engine::pause_game() noexcept
{
    std::scoped_lock lock(mutex);
    world.pause();
}
