#include <memory>

#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <android/log.h>

#include "engine.hpp"

std::unique_ptr<Engine> engine;

#define DISP_DATA_FIELDS \
X(ObservableInt, set_int, score) \
X(ObservableInt, set_int, high_score)\
X(ObservableFloat, set_float, grav_angle)\
X(ObservableInt, set_int, pressure)

struct JVM_refs
{
    void init(JNIEnv * env, jobject activity, jobject resources_local) noexcept
    {
        if(env->GetJavaVM(&vm) != 0)
            __android_log_assert("Couldn't get java VM", "JNI", NULL);

        // get game win / game over methods
        main_activity = env->NewGlobalRef(activity);
        if(!main_activity)
            __android_log_assert("Couldn't get activity global ref", "JNI", NULL);

        jclass main_activity_class = env->GetObjectClass(main_activity);
        if(!main_activity_class)
            __android_log_assert("Couldn't get 'MainActivity' class", "JNI", NULL);

        game_win_method = env->GetMethodID(main_activity_class, "game_win", "(IZ)V");
        if(!game_win_method)
            __android_log_assert("Couldn't get 'game_win' method", "JNI", NULL);

        game_over_method = env->GetMethodID(main_activity_class, "game_over", "(IZ)V");
        if(!game_over_method)
            __android_log_assert("Couldn't get 'game_over' method", "JNI", NULL);

        game_pause_method = env->GetMethodID(main_activity_class, "game_pause", "()V");
        if(!game_pause_method)
            __android_log_assert("Couldn't get 'game_pause' method", "JNI", NULL);

        achievement_method = env->GetMethodID(main_activity_class, "achievement", "(I)V");
        if(!achievement_method)
            __android_log_assert("Couldn't get 'achievement' method", "JNI", NULL);

        // get Resource classes and methods
        resources = env->NewGlobalRef(resources_local);
        if(!resources)
            __android_log_assert("Couldn't get resources global ref", "JNI", NULL);

        get_int_array_method = env->GetMethodID(env->GetObjectClass(resources), "getIntArray", "(I)[I");
        if(!get_int_array_method)
            __android_log_assert("Couldn't get 'get_int_array' method", "JNI", NULL);

        jclass R_array_local = env->FindClass("org/mattvchandler/a2050/R$array");
        if(!R_array_local)
            __android_log_assert("Couldn't get 'R.array' class", "JNI", NULL);

        R_array = static_cast<jclass>(env->NewGlobalRef(R_array_local));
        if(!R_array)
            __android_log_assert("Couldn't get 'R.array' global ref", "JNI", NULL);

        jclass R_color_local = env->FindClass("org/mattvchandler/a2050/R$color");
        if(!R_color_local)
            __android_log_assert("Couldn't get 'R.color' class", "JNI", NULL);

        R_color = static_cast<jclass>(env->NewGlobalRef(R_color_local));
        if(!R_color)
            __android_log_assert("Couldn't get 'R.color' global ref", "JNI", NULL);

        jclass context_compat_local = env->FindClass("android/support/v4/content/ContextCompat");
        if(!context_compat_local)
            __android_log_assert("Couldn't get 'android.support.v4.content.ContextCompat' class", "JNI", NULL);

        context_compat = static_cast<jclass>(env->NewGlobalRef(context_compat_local));
        if(!context_compat)
            __android_log_assert("Couldn't get 'contextCompat' global ref", "JNI", NULL);

        get_color_method = env->GetStaticMethodID(context_compat_local, "getColor", "(Landroid/content/Context;I)I");
        if(!get_color_method)
            __android_log_assert("Couldn't get 'ContextCompat.getColor' method", "JNI", NULL);

        // get DispData class fields, and set methods for each
        jclass observable_int = env->FindClass("android/databinding/ObservableInt");
        if(!observable_int)
            __android_log_assert("Couldn't get 'ObservableInt' class", "JNI", NULL);

        set_int = env->GetMethodID(observable_int, "set", "(I)V");
        if(!set_int)
            __android_log_assert("Couldn't get 'ObservableInt.set' method", "JNI", NULL);

        jclass observable_float = env->FindClass("android/databinding/ObservableFloat");
        if(!observable_float)
            __android_log_assert("Couldn't get 'ObservableFloat' class", "JNI", NULL);

        set_float = env->GetMethodID(observable_float, "set", "(F)V");
        if(!set_float)
            __android_log_assert("Couldn't get 'ObservableFloat.set' method", "JNI", NULL);

        jclass DispData = env->FindClass("org/mattvchandler/a2050/MainActivity$DispData");
        if(!DispData)
            __android_log_assert("Couldn't get 'DispData' class", "JNI", NULL);

#define X(type, method, name) \
            name = env->GetFieldID(DispData, #name, "Landroid/databinding/" #type ";");\
            if(!name)\
                __android_log_assert("Couldn't get '" #name "' field", "JNI", NULL);
        DISP_DATA_FIELDS
#undef X
    }

    void destroy(JNIEnv * env) noexcept
    {
        env->DeleteGlobalRef(main_activity);
        env->DeleteGlobalRef(resources);
        env->DeleteGlobalRef(R_array);
        env->DeleteGlobalRef(R_color);
        env->DeleteGlobalRef(context_compat);

        vm                   = nullptr;
        main_activity        = nullptr;
        game_win_method      = nullptr;
        game_over_method     = nullptr;
        game_pause_method    = nullptr;
        achievement_method   = nullptr;
        resources            = nullptr;
        R_array              = nullptr;
        R_color              = nullptr;
        context_compat       = nullptr;
        get_int_array_method = nullptr;
        get_color_method     = nullptr;
        set_int              = nullptr;
        set_float            = nullptr;
#define X(type, method, name) name = nullptr;
        DISP_DATA_FIELDS
#undef X
    }

    JavaVM * vm                    = nullptr;

    jobject   main_activity        = nullptr;
    jmethodID game_win_method      = nullptr;
    jmethodID game_over_method     = nullptr;
    jmethodID game_pause_method    = nullptr;
    jmethodID achievement_method   = nullptr;

    jobject   resources            = nullptr;
    jclass    R_array              = nullptr;
    jclass    R_color              = nullptr;
    jclass    context_compat       = nullptr;
    jmethodID get_int_array_method = nullptr;
    jmethodID get_color_method     = nullptr;

    jmethodID set_int              = nullptr;
    jmethodID set_float            = nullptr;

#define X(type, method, name) jfieldID name = nullptr;
    DISP_DATA_FIELDS
#undef X
};

JVM_refs jvm_refs;

struct Persists
{
    bool first_run = true;
};
Persists persists;

class Java_thread_env
{
private:
    JNIEnv *env = nullptr;
    bool attached = false;

public:
    Java_thread_env()
    {
        if(!jvm_refs.vm)
            return;

        if(jvm_refs.vm->GetEnv((void **)&env, JNI_VERSION_1_6) == JNI_OK)
            attached = true;
        else if(jvm_refs.vm->AttachCurrentThread(&env, NULL) != JNI_OK)
            __android_log_assert("could not attach thread!", "JNI::Java_thread_env", NULL);
    }
    ~Java_thread_env()
    {
        if(!attached)
            jvm_refs.vm->DetachCurrentThread();
    }

    Java_thread_env(const Java_thread_env &) = delete;
    Java_thread_env & operator=(const Java_thread_env &) = delete;

    JNIEnv *operator->() { return env; }
};

void game_win(int score, bool new_high_score)
{
    Java_thread_env env;

    env->CallVoidMethod(jvm_refs.main_activity, jvm_refs.game_win_method, score, new_high_score);
}
void game_over(int score, bool new_high_score)
{
    Java_thread_env env;

    env->CallVoidMethod(jvm_refs.main_activity, jvm_refs.game_over_method, score, new_high_score);
}
void game_pause()
{
    Java_thread_env env;

    env->CallVoidMethod(jvm_refs.main_activity, jvm_refs.game_pause_method);
}
void achievement(int size)
{
    Java_thread_env env;

    env->CallVoidMethod(jvm_refs.main_activity, jvm_refs.achievement_method, size);
}

std::vector<int> get_res_int_array(const std::string & id)
{
    Java_thread_env env;

    jfieldID id_field = env->GetStaticFieldID(jvm_refs.R_array, id.c_str(), "I");
    if(!id_field)
    {
        __android_log_print(ANDROID_LOG_ERROR, "get_res_int_array", "Could not find resource array: %s", id.c_str());
        return {};
    }

    int array_id = env->GetStaticIntField(jvm_refs.R_array, id_field);

    jintArray j_arr = static_cast<jintArray>(env->CallObjectMethod(jvm_refs.resources, jvm_refs.get_int_array_method, array_id));
    int * c_arr = env->GetIntArrayElements(j_arr, NULL);
    jsize c_arr_size = env->GetArrayLength(j_arr);

    std::vector<int> ret(c_arr, c_arr + c_arr_size);

    env->ReleaseIntArrayElements(j_arr, c_arr, 0);

    return ret;
}
int get_res_color(const std::string & id)
{
    Java_thread_env env;

    jfieldID id_field = env->GetStaticFieldID(jvm_refs.R_color, id.c_str(), "I");
    if(!id_field)
    {
        __android_log_print(ANDROID_LOG_ERROR, "get_res_color", "Could not find resource color: %s", id.c_str());
        return {};
    }

    int color_id = env->GetStaticIntField(jvm_refs.R_color, id_field);

    return env->CallStaticIntMethod(jvm_refs.context_compat, jvm_refs.get_color_method, jvm_refs.main_activity, color_id);
}

extern "C"
{
JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_create(JNIEnv * env, jobject activity, jobject assetManager, jstring path, jobject resources_local, jboolean gravity_mode)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "create");
    if(engine)
        __android_log_assert("create called after engine initialized", "JNI", NULL);

    jvm_refs.init(env, activity, resources_local);

    const char * data_path = env->GetStringUTFChars(path, NULL);

    engine = std::make_unique<Engine>(AAssetManager_fromJava(env, assetManager), data_path, persists.first_run, gravity_mode);

    persists.first_run = false;

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

    jvm_refs.destroy(env);
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
    engine->pause_game(true);
}
JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_unpause(JNIEnv *, jobject)
{
    __android_log_print(ANDROID_LOG_DEBUG, "JNI", "unpause");
    if(!engine)
        __android_log_assert("unpause called before engine initialized", "JNI", NULL);
    engine->unpause();
}
JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_getUIData(JNIEnv * env, jobject, jobject dispdata)
{
    if(!engine)
        __android_log_assert("getUIData called before engine initialized", "JNI", NULL);

    auto data = engine->get_ui_data();
#define X(type, method, name) \
        jobject name = env->GetObjectField(dispdata, jvm_refs.name);\
        env->CallVoidMethod(name, jvm_refs.method, data.name);
    DISP_DATA_FIELDS
#undef X



}
}
