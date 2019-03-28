// Copyright 2019 Matthew Chandler

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "engine.hpp"
#include "world.hpp"

#include <chrono>
#include <fstream>

#include <GLES2/gl2.h>

#include "log.hpp"

using namespace std::chrono_literals;

#define CASE_STR( value ) case value: return #value;
const char* eglGetErrorString( EGLint error )
{
    switch(error)
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
    LOG_DEBUG_WRITE("Engine::destroy_egl", "Destroying display");

    if(display != EGL_NO_DISPLAY)
    {
        if(eglGetCurrentContext() != EGL_NO_CONTEXT)
            eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if(context != EGL_NO_CONTEXT)
            eglDestroyContext(display, context);

        if(surface != EGL_NO_SURFACE)
            eglDestroySurface(display, surface);

        eglTerminate(display);
    }

    display = EGL_NO_DISPLAY;
    surface = EGL_NO_SURFACE;
    context = EGL_NO_CONTEXT;
};

bool Engine::init_egl()
{
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if(display == EGL_NO_DISPLAY)
    {
        LOG_ERROR_PRINT("Engine::init_egl", "eglGetDisplay error: %s", eglGetErrorString(eglGetError()));
        return false;
    }

    if(!eglInitialize(display, nullptr, nullptr))
    {
        LOG_ERROR_PRINT("Engine::init_egl", "eglGetInitialize error: %s", eglGetErrorString(eglGetError()));
        return false;
    }

    LOG_DEBUG_WRITE("Engine::init_egl", "egl initialized");
    return true;
}
bool Engine::init_surface()
{
    if(!has_surface)
        return false;

    EGLint native_visual_id;
    if(!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &native_visual_id))
    {
        LOG_ERROR_PRINT("Engine::init_surface", "eglConfigAttrib error: %s", eglGetErrorString(eglGetError()));
        return false;
    }

    ANativeWindow_setBuffersGeometry(win, 0, 0, native_visual_id);

    surface = eglCreateWindowSurface(display, config, win, nullptr);
    if(surface == EGL_NO_SURFACE)
    {
        LOG_ERROR_PRINT("Engine::init_surface", "eglCreateWindowSurface error: %s", eglGetErrorString(eglGetError()));
        return false;
    }

    if(!eglQuerySurface(display, surface, EGL_WIDTH, &width) || !eglQuerySurface(display, surface, EGL_HEIGHT, &height))
    {
        LOG_ERROR_PRINT("Engine::init_surface", "eglQuerySurface error: %s", eglGetErrorString(eglGetError()));
        eglDestroySurface(display, surface);
        return false;
    }

    if(height <= 0 || width <= 0)
    {
        __android_log_print(ANDROID_LOG_WARN, "Engine::init_surface", "eglQuerySurface returned invalid size: %d x %d", width, height);
        eglDestroySurface(display, surface);
        return false;
    }

    LOG_DEBUG_WRITE("Engine::init_surface", "surface created");
    return true;
}
bool Engine::init_context()
{
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
        LOG_ERROR_PRINT("Engine::init_context", "eglChooseConfig error: %s", eglGetErrorString(eglGetError()));
        return false;
    }

    EGLint context_version[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_version);
    if(context == EGL_NO_CONTEXT)
    {
        LOG_ERROR_PRINT("Engine::init_context", "eglCreateContext error: %s", eglGetErrorString(eglGetError()));
        return false;
    }

    LOG_DEBUG_WRITE("Engine::init_context", "context created");

    return true;
}

bool Engine::can_render()
{
    if(display == EGL_NO_DISPLAY)
    {
        LOG_DEBUG_WRITE("Engine::can_render", "can't render: no display");
        return false;
    }

    if(!win)
    {
        LOG_DEBUG_WRITE("Engine::can_render", "can't render: no window");
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
                    LOG_DEBUG_WRITE("Engine::can_render", "can't render: couldn't init context");
                    return false;
                }
            }

            if(!has_surface)
            {
                LOG_DEBUG_WRITE("Engine::can_render", "can't render: no surface");
                return false;
            }

            if(!init_surface())
            {
                LOG_DEBUG_WRITE("Engine::can_render", "can't render: couldn't init surface");
                return false;
            }
        }

        if(!eglMakeCurrent(display, surface, surface, context))
        {
            LOG_ERROR_PRINT("Engine::can_render", "eglMakeCurrent error: %s", eglGetErrorString(eglGetError()));
            return false;
        }

        LOG_DEBUG_WRITE("Engine::can_render", "set up to render");

        glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if(!eglSwapBuffers(display, surface))
        {
            LOG_ERROR_WRITE("Engine::can_render", "couldn't swap");
        }

        world.init();
        world.resize(width, height);
    }

    return true;
}

void Engine::render_loop()
{
    LOG_DEBUG_WRITE("Engine::render_loop", "start render loop");

    mutex.lock();
    init_egl();
    mutex.unlock();

    const std::chrono::duration<float> target_frametime{ 1.0f / 30.0f};

    bool must_render = true;
    while(running)
    {
        mutex.lock();
        auto frame_start_time = std::chrono::steady_clock::now();

        if(!can_render())
        {
            mutex.unlock();
            std::this_thread::sleep_until(frame_start_time + target_frametime);
            continue;
        }

        int new_width, new_height;
        if(!eglQuerySurface(display, surface, EGL_WIDTH, &new_width) || !eglQuerySurface(display, surface, EGL_HEIGHT, &new_height))
        {
            LOG_ERROR_PRINT("Engine::render_loop", "eglQuerySurface error: %s", eglGetErrorString(eglGetError()));
        }

        if(new_width <= 0 || new_height <= 0)
        {
            mutex.unlock();
            std::this_thread::sleep_until(frame_start_time + target_frametime);
            continue;
        }

        if(new_width != width || new_height != height)
        {
            width = new_width; height = new_height;
            world.resize(width, height);
            must_render = true;
        }
        else
        {
            // for some reason EGL surface doesn't update its size until we've drawn to it a couple of times. force that drawing even if paused
            int win_width = ANativeWindow_getWidth(win);
            int win_height = ANativeWindow_getHeight(win);

            if(win_width != width || win_height != height)
                must_render = true;
        }

        if(!world.is_paused())
            must_render = true;

        if(must_render)
        {
            must_render = world.render();

            if(!eglSwapBuffers(display, surface))
            {
                LOG_ERROR_WRITE("Engine::render_loop", "couldn't swap");
            }
        }
        mutex.unlock();
        std::this_thread::sleep_until(frame_start_time + target_frametime);
    }

    world.destroy();
    destroy_egl();

    LOG_DEBUG_WRITE("Engine::render_loop", "end render loop");
}

void Engine::physics_loop()
{
    LOG_DEBUG_WRITE("Engine::physics_loop", "start physics loop");

    if(gravity_mode)
    {
        sensor_queue = ASensorManager_createEventQueue(sensor_mgr, ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS), sensor_ident, nullptr, nullptr);
        if(!sensor_queue)
            __android_log_assert("Could not prepare queue for gravity sensor", "Engine::physics_loop", nullptr);

        accelerometer_sensor = ASensorManager_getDefaultSensor(sensor_mgr, ASENSOR_TYPE_ACCELEROMETER);

        if(!accelerometer_sensor)
            __android_log_assert("Could not get Accelerometer Sensor", "Engine::physics_loop", nullptr);

        ASensorEventQueue_enableSensor(sensor_queue, accelerometer_sensor);
    }

    const std::chrono::duration<float> target_frametime{10ms};
    auto last_frame_time = std::chrono::steady_clock::now() - target_frametime;

    while(running)
    {
        mutex.lock();
        auto frame_start_time = std::chrono::steady_clock::now();
        auto dt = std::chrono::duration_cast<std::chrono::duration<float>>(frame_start_time - last_frame_time).count();
        dt = std::min(dt, target_frametime.count() * 1.5f);
        last_frame_time = frame_start_time;

        if(gravity_mode)
        {
            int ident = ALooper_pollOnce(0, nullptr, nullptr, nullptr);
            if(ident == sensor_ident)
            {
                ASensorEvent sensorEvent;
                ASensorEventQueue_getEvents(sensor_queue, &sensorEvent, 1);
                grav_sensor_vec.x =  sensorEvent.vector.x;
                grav_sensor_vec.y = -sensorEvent.vector.y;

                // reorient gravity to current screen rotation
                switch(rotation)
                {
                case Rotation::ROTATION_0:
                    break;
                case Rotation::ROTATION_90:
                    grav_sensor_vec = {grav_sensor_vec.y, -grav_sensor_vec.x};
                    break;
                case Rotation::ROTATION_180:
                    grav_sensor_vec = {-grav_sensor_vec.x, -grav_sensor_vec.y};
                    break;
                case Rotation::ROTATION_270:
                    grav_sensor_vec = {-grav_sensor_vec.y, grav_sensor_vec.x};
                    break;
                }
            }
        }

        world.physics_step(dt, grav_sensor_vec);

        mutex.unlock();
        std::this_thread::sleep_until(frame_start_time + target_frametime);
    }

    if(gravity_mode)
    {
        if(ASensorEventQueue_disableSensor(sensor_queue, accelerometer_sensor) != 0)
            __android_log_assert("Could not disable gravity sensor", "Engine::physics_loop", nullptr);

        if(ASensorManager_destroyEventQueue(sensor_mgr, sensor_queue) != 0)
            __android_log_assert("Could not destroy gravity sensor queue", "Engine::physics_loop", nullptr);
    }

    LOG_DEBUG_WRITE("Engine::physics_loop", "end physics loop");
}

Engine::Engine(AAssetManager * asset_manager, const std::string & data_path, bool first_run, bool gravity_mode, Rotation rotation):
        data_path(data_path),
        gravity_mode(gravity_mode),
        rotation(rotation),
        world(asset_manager, gravity_mode)
{
    std::ifstream savefile(data_path + "/save.json");
    if(savefile)
    {
        nlohmann::json data;
        savefile>>data;
        world.deserialize(data, first_run);
    }

#if __ANDROID_API__ >= __ANDROID_API_O__
    sensor_mgr = ASensorManager_getInstanceForPackage("2050");
#else
    sensor_mgr = ASensorManager_getInstance();
#endif
    if(!sensor_mgr)
        __android_log_assert("Could not get ASensorManager", "Engine::Engine", nullptr);
}

void Engine::resume() noexcept
{
    running = true;
    render_thread = std::thread(&Engine::render_loop, this);
    physics_thread = std::thread(&Engine::physics_loop, this);
}
void Engine::pause() noexcept
{
    running = false;
    render_thread.join();
    physics_thread.join();
}
void Engine::stop() noexcept
{
    try
    {
        std::ofstream savefile(data_path + "/save.json");
        savefile<<world.serialize();

        LOG_DEBUG_WRITE("Engine::stop", "saved data");
    }
    catch(nlohmann::json::type_error & e)
    {
        LOG_ERROR_PRINT("Engine::stop", "could not serialize data: %s", e.what());
    }
    catch(...)
    {
        LOG_ERROR_WRITE("Engine::stop", "unknown exception when opening savefile");
    }
}
void Engine::set_focus(bool focus) noexcept
{
    LOG_DEBUG_PRINT("Engine::set_focus", "focused: %s", focus ? "true" : "false");
    if(focus)
        unpause();
    else
        pause_game();
}

void Engine::surface_changed(ANativeWindow *window) noexcept
{
    std::scoped_lock lock(mutex);
    if(window)
    {
        LOG_DEBUG_PRINT("Engine::surface_changed", "surfaceChanged, with size: %d x %d", ANativeWindow_getWidth(window), ANativeWindow_getHeight(window));
        has_surface = true;
        win = window;
    }
    else
    {
        has_surface = false;
        ANativeWindow_release(win);
        win = nullptr;
    }
}

void Engine::fling(float x, float y) noexcept
{
    std::scoped_lock lock(mutex);
    world.fling(x, y);
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
void Engine::unpause() noexcept
{
    std::scoped_lock lock(mutex);
    world.unpause();
}
bool Engine::is_paused() noexcept
{
    std::scoped_lock lock(mutex);
    return world.is_paused();
}

World::UI_data Engine::get_ui_data() noexcept
{
    std::scoped_lock lock(mutex);
    return world.get_ui_data();
}
