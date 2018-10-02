// Copyright 2018 Matthew Chandler

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

#ifndef INC_2050_ENGINE_HPP
#define INC_2050_ENGINE_HPP

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#include <android/asset_manager.h>
#include <android/native_window.h>
#include <android/sensor.h>

#include <EGL/egl.h>

#include "world.hpp"

// Shadows android.View.Surface.ROTATION_*
enum class Rotation: int{ROTATION_0, ROTATION_90, ROTATION_180, ROTATION_270};

class Engine
{
private:
    ANativeWindow * win = nullptr;
    std::string data_path;

    int width = 0, height = 0;

    std::atomic<bool> has_surface = false;

    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    EGLConfig config;

    const bool gravity_mode = false;
    const Rotation rotation = Rotation::ROTATION_0;
    ASensorManager * sensor_mgr;
    const ASensor * accelerometer_sensor;
    ASensorEventQueue * sensor_queue;
    const int sensor_ident = 1;
    glm::vec2 grav_sensor_vec {0.0f, -1.0f};

    std::thread render_thread;
    std::thread physics_thread;
    std::atomic<bool> running = false;

    World world;

    std::mutex mutex;

    void destroy_egl();
    bool init_egl();
    bool init_context();
    bool init_surface();
    bool can_render();

    void render_loop();
    void physics_loop();

public:
    Engine(AAssetManager * asset_manager, const std::string & data_path, bool first_run, bool gravity_mode, Rotation rotation);

    void resume() noexcept;
    void pause() noexcept;
    void stop() noexcept;

    void set_focus(bool focus) noexcept;
    void surface_changed(ANativeWindow * window) noexcept;

    void fling(float x, float y) noexcept;

    void new_game() noexcept;
    void pause_game() noexcept;
    void unpause() noexcept;
    bool is_paused() noexcept;
    World::UI_data get_ui_data() noexcept;
};


#endif //INC_2050_ENGINE_HPP
