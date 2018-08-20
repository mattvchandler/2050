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
    ASensorManager * sensor_mgr;
    const ASensor * accelerometer_sensor;
    ASensorEventQueue * sensor_queue;
    const int sensor_ident = 1;
    glm::vec3 grav_sensor_vec = glm::vec3(-1.0f, 0.0f, 0.0f);

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
    Engine(AAssetManager * asset_manager, const std::string & data_path, bool first_run, bool gravity_mode);

    void start() noexcept;
    void resume() noexcept;
    void pause() noexcept;
    void stop() noexcept;

    void set_focus(bool focus) noexcept;
    void surface_changed(ANativeWindow * window) noexcept;

    void fling(float x, float y) noexcept;

    void new_game() noexcept;
    void pause_game(bool show_dialog = false) noexcept;
    void unpause() noexcept;
    bool is_paused() noexcept;
    World::UI_data get_ui_data() noexcept;
};


#endif //INC_2050_ENGINE_HPP
