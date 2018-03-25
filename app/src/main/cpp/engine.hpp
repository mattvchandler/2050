#ifndef INC_2050_ENGINE_HPP
#define INC_2050_ENGINE_HPP

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#include <android/asset_manager.h>
#include <android/native_window.h>

#include <EGL/egl.h>

#include "world.hpp"

class Engine
{
private:
    ANativeWindow * win = nullptr;
    AAssetManager * asset_manager = nullptr;
    std::string data_path;

    int width = 0, height = 0;

    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;

    std::thread render_thread;
    std::thread physics_thread;
    std::atomic<bool> running = false;

    World world;

    bool init_display() noexcept;
    void destroy_display() noexcept;

    std::mutex lock;
    void render_loop() noexcept;
    void physics_loop() noexcept;

public:
    void set_window(ANativeWindow * window) noexcept;
    void set_asset_manager(AAssetManager * manager) noexcept;
    void set_data_path(const std::string & path) noexcept;

    void start() noexcept;
    void stop() noexcept;

    void fling(float x, float y);
};


#endif //INC_2050_ENGINE_HPP
