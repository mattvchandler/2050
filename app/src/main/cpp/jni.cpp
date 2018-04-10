#include <memory>

#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <android/log.h>

#include "engine.hpp"

std::unique_ptr<Engine> engine;

#define DISP_DATA_FIELDS \
X(ObservableInt, set_int, score) \
X(ObservableInt, set_int, high_score)\
X(ObservableFloat, set_float, grav_angle)

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

        // get R.string class and Resources.getString() method
        resources = env->NewGlobalRef(resources_local);
        if(!resources)
            __android_log_assert("Couldn't get resources global ref", "JNI", NULL);

        get_string_method = env->GetMethodID(env->GetObjectClass(resources), "getString", "(I)Ljava/lang/String;");
        if(!get_string_method)
            __android_log_assert("Couldn't get 'get_string' method", "JNI", NULL);

        jclass R_string_local = env->FindClass("org/mattvchandler/a2050/R$string");
        if(!R_string_local)
            __android_log_assert("Couldn't get 'R.string' class", "JNI", NULL);

        R_string = static_cast<jclass>(env->NewGlobalRef(R_string_local));
        if(!R_string)
            __android_log_assert("Couldn't get 'R.string' global ref", "JNI", NULL);

        // get DispData class fields, and set methods for each
        jclass observable_int = env->FindClass("android/databinding/ObservableInt");
        if(!observable_int)
            __android_log_assert("Couldn't get 'ObservableInt' field", "JNI", NULL);

        set_int = env->GetMethodID(observable_int, "set", "(I)V");
        if(!set_int)
            __android_log_assert("Couldn't get 'ObservableInt.set' method", "JNI", NULL);

        jclass observable_float = env->FindClass("android/databinding/ObservableFloat");
        if(!observable_float)
            __android_log_assert("Couldn't get 'ObservableFloat' field", "JNI", NULL);

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
        env->DeleteGlobalRef(R_string);

        vm                = nullptr;
        main_activity     = nullptr;
        game_win_method   = nullptr;
        game_over_method  = nullptr;
        game_pause_method = nullptr;
        resources         = nullptr;
        R_string          = nullptr;
        get_string_method = nullptr;
        set_int           = nullptr;
        set_float         = nullptr;
#define X(type, method, name) name = nullptr;
        DISP_DATA_FIELDS
#undef X
    }

    JavaVM * vm                 = nullptr;

    jobject   main_activity     = nullptr;
    jmethodID game_win_method   = nullptr;
    jmethodID game_over_method  = nullptr;
    jmethodID game_pause_method = nullptr;

    jobject   resources         = nullptr;
    jclass    R_string          = nullptr;
    jmethodID get_string_method = nullptr;

    jmethodID set_int           = nullptr;
    jmethodID set_float         = nullptr;

#define X(type, method, name) jfieldID name = nullptr;
    DISP_DATA_FIELDS
#undef X
};

JVM_refs jvm_refs;

void game_win(int score, bool new_high_score)
{
    bool attached = false;
    JNIEnv * env;
    if(jvm_refs.vm->GetEnv((void **)&env, JNI_VERSION_1_6) == JNI_OK)
        attached = true;
    else if(jvm_refs.vm->AttachCurrentThread(&env, NULL) != JNI_OK)
        __android_log_assert("could not attach thread!", "JNI::game_win", NULL);

    env->CallVoidMethod(jvm_refs.main_activity, jvm_refs.game_win_method, score, new_high_score);

    if(!attached)
    jvm_refs.vm->DetachCurrentThread();
}
void game_over(int score, bool new_high_score)
{
    bool attached = false;
    JNIEnv * env;
    if(jvm_refs.vm->GetEnv((void **)&env, JNI_VERSION_1_6) == JNI_OK)
        attached = true;
    else if(jvm_refs.vm->AttachCurrentThread(&env, NULL) != JNI_OK)
        __android_log_assert("could not attach thread!", "JNI::game_over", NULL);

    env->CallVoidMethod(jvm_refs.main_activity, jvm_refs.game_over_method, score, new_high_score);

    if(!attached)
        jvm_refs.vm->DetachCurrentThread();
}
void game_pause()
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "pause_game");
    bool attached = false;
    JNIEnv * env;
    if(jvm_refs.vm->GetEnv((void **)&env, JNI_VERSION_1_6) == JNI_OK)
        attached = true;
    else if(jvm_refs.vm->AttachCurrentThread(&env, NULL) != JNI_OK)
        __android_log_assert("could not attach thread!", "JNI::game_pause", NULL);

    env->CallVoidMethod(jvm_refs.main_activity, jvm_refs.game_pause_method);

    if(!attached)
        jvm_refs.vm->DetachCurrentThread();
}

std::string get_res_string(const std::string & id)
{
    bool attached = false;
    JNIEnv * env;
    if(jvm_refs.vm->GetEnv((void **)&env, JNI_VERSION_1_6) == JNI_OK)
        attached = true;
    else if(jvm_refs.vm->AttachCurrentThread(&env, NULL) != JNI_OK)
        __android_log_assert("could not attach thread!", "JNI::test", NULL);

    jfieldID id_field = env->GetStaticFieldID(jvm_refs.R_string, id.c_str(), "I");
    if(!id_field)
    {
        __android_log_print(ANDROID_LOG_ERROR, "get_res_string", "Could not find resource string: %s", id.c_str());
        return "";
    }

    int string_id = env->GetStaticIntField(jvm_refs.R_string, id_field);

    jstring j_str = static_cast<jstring>(env->CallObjectMethod(jvm_refs.resources, jvm_refs.get_string_method, string_id));
    const char * c_str = env->GetStringUTFChars(j_str, NULL);
    std::string cpp_str = c_str;

    env->ReleaseStringUTFChars(j_str, c_str);

    if(!attached)
        jvm_refs.vm->DetachCurrentThread();

    return cpp_str;
}

extern "C"
{
JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_create(JNIEnv * env, jobject activity, jobject assetManager, jstring path, jobject resources_local)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "create");
    if(engine)
        __android_log_assert("create called after engine initialized", "JNI", NULL);

    jvm_refs.init(env, activity, resources_local);

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
JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_pause(JNIEnv *, jobject, jboolean screen_on)
{
    __android_log_write(ANDROID_LOG_DEBUG, "JNI", "pause");
    if(!engine)
        __android_log_assert("pause called before engine initialized", "JNI", NULL);
    engine->pause(screen_on);
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
    engine->pause_game();

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
