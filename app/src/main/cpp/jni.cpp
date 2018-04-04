#include <memory>

#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <android/log.h>

#include "engine.hpp"

std::unique_ptr<Engine> engine;

JavaVM * vm = nullptr;
jobject main_activity;
jmethodID game_win_method;
jmethodID game_over_method;

void game_win(int score, bool new_high_score)
{
    bool attached = false;
    JNIEnv * my_env;
    if(vm->GetEnv((void **)&my_env, JNI_VERSION_1_6) == JNI_OK)
        attached = true;
    else if(vm->AttachCurrentThread(&my_env, NULL) != JNI_OK)
        __android_log_assert("could not attach thread!", "JNI::test", NULL);

    my_env->CallVoidMethod(main_activity, game_win_method, score, new_high_score);

    if(!attached)
    vm->DetachCurrentThread();
}
void game_over(int score, bool new_high_score)
{
    bool attached = false;
    JNIEnv * my_env;
    if(vm->GetEnv((void **)&my_env, JNI_VERSION_1_6) == JNI_OK)
        attached = true;
    else if(vm->AttachCurrentThread(&my_env, NULL) != JNI_OK)
        __android_log_assert("could not attach thread!", "JNI::test", NULL);

    my_env->CallVoidMethod(main_activity, game_over_method, score, new_high_score);

    if(!attached)
        vm->DetachCurrentThread();
}

extern "C"
{
JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_create(JNIEnv * env, jobject activity, jobject assetManager, jstring path)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "create");
    if(engine)
        __android_log_assert("create called after engine initialized", "JNI", NULL);

    if(env->GetJavaVM(&vm) != 0)
        __android_log_assert("Couldn't get java VM", "JNI", NULL);

    main_activity = env->NewGlobalRef(activity);
    if(!main_activity)
        __android_log_assert("Couldn't get activity", "JNI", NULL);

    game_win_method = env->GetMethodID(env->GetObjectClass(main_activity), "game_win", "(IZ)V");
    if(!game_win_method)
        __android_log_assert("Couldn't get 'game_win' method", "JNI", NULL);

    game_over_method = env->GetMethodID(env->GetObjectClass(main_activity), "game_over", "(IZ)V");
    if(!game_over_method)
        __android_log_assert("Couldn't get 'game_over' method", "JNI", NULL);

    const char * data_path = env->GetStringUTFChars(path, NULL);

    engine = std::make_unique<Engine>(AAssetManager_fromJava(env, assetManager), data_path);

    env->ReleaseStringUTFChars(path, data_path);
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
JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_destroy(JNIEnv * env, jobject)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "destroy");
    if(!engine)
        __android_log_assert("destroy called before engine initialized", "JNI", NULL);
    engine.reset();

    env->DeleteGlobalRef(main_activity);
}

JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_focus(JNIEnv * env, jobject, jboolean has_focus)
{
    __android_log_print(ANDROID_LOG_DEBUG, "JNI", "focus: %s", has_focus? "true" : "false");
    if(!engine)
        __android_log_assert("focus called before engine initialized", "JNI", NULL);
    engine->set_focus(has_focus);
}

JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_surfaceChanged(JNIEnv * env, jobject, jobject surface)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "surfaceChanged");
    if(!engine)
        __android_log_assert("surfaceChanged called before engine initialized", "JNI", NULL);
    if(surface)
        engine->surface_changed(ANativeWindow_fromSurface(env, surface));
    else
        engine->surface_changed(nullptr);
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
JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_newGame(JNIEnv *, jobject)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "newGame");
    if(!engine)
        __android_log_assert("newGame called before engine initialized", "JNI", NULL);
    engine->new_game();
}
JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_pauseGame(JNIEnv *, jobject)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "pauseGame");
    if(!engine)
        __android_log_assert("pauseGame called before engine initialized", "JNI", NULL);
    engine->pause_game();
}
}
