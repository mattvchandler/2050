#include <memory>

#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <android/log.h>

#include <jni.h>

#include "engine.hpp"

std::unique_ptr<Engine> engine;

extern "C"
{
JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_create(JNIEnv * env, jobject, jobject assetManager, jstring path)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "create");
    if(engine)
        __android_log_assert("create called after engine initialized", "JNI", NULL);

    engine = std::make_unique<Engine>(AAssetManager_fromJava(env, assetManager), env->GetStringUTFChars(path, NULL));
}

JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_start(JNIEnv *, jobject)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "start");
    if(!engine)
        __android_log_assert("start called before engine initialized", "JNI", NULL);
    engine->start();
}

JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_resume(JNIEnv *, jobject)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "resume");
    if(!engine)
        __android_log_assert("resume called before engine initialized", "JNI", NULL);
    engine->resume();
}
JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_pause(JNIEnv *, jobject)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "pause");
    if(!engine)
        __android_log_assert("pause called before engine initialized", "JNI", NULL);
    engine->pause();
}

JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_stop(JNIEnv *, jobject)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "stop");
    if(!engine)
        __android_log_assert("stop called before engine initialized", "JNI", NULL);
    engine->stop();
}
JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_destroy(JNIEnv *, jobject)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "destroy");
    if(!engine)
        __android_log_assert("destroy called before engine initialized", "JNI", NULL);
    engine.reset();
}

JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_focus(JNIEnv * env, jobject, jboolean has_focus)
{
    __android_log_print(ANDROID_LOG_DEBUG, "JNI", "focus: %s", has_focus? "true" : "false");
    if(!engine)
        __android_log_assert("focus called before engine initialized", "JNI", NULL);
    engine->set_focus(has_focus);
}

// TODO: probably only pass the window in surfaceChanged
JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_surfaceCreated(JNIEnv * env, jobject, jobject surface)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "surfaceCreated");
    if(!engine)
        __android_log_assert("surfaceCreated called before engine initialized", "JNI", NULL);
    if(surface)
        engine->surface_created(ANativeWindow_fromSurface(env, surface));

}
JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_surfaceDestroyed(JNIEnv * env, jobject)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "surfaceDestroyed");
    if(!engine)
        __android_log_assert("surfaceDestroyed called before engine initialized", "JNI", NULL);
    engine->surface_destroyed();
}
JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_surfaceChanged(JNIEnv * env, jobject, jobject surface)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "surfaceChanged");
    if(!engine)
        __android_log_assert("surfaceChanged called before engine initialized", "JNI", NULL);
    if(surface)
        engine->surface_changed(ANativeWindow_fromSurface(env, surface));
}

JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_fling(JNIEnv *, jobject, jfloat x, jfloat y)
{
    __android_log_print(ANDROID_LOG_DEBUG, "JNI", "fling [%f, %f]", (float)x, (float)y);
    if(!engine)
        __android_log_assert("fling called before engine initialized", "JNI", NULL);
    engine->fling(x, y);
}
JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_tap(JNIEnv *, jobject, jfloat x, jfloat y)
{
    __android_log_print(ANDROID_LOG_DEBUG, "JNI", "tap [%f, %f]", (float)x, (float)y);
    if(!engine)
        __android_log_assert("fling called before engine initialized", "JNI", NULL);
    engine->tap(x, y);
}
}
