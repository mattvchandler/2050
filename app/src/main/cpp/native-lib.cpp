#include <jni.h>
#include <GLES3/gl3.h>

extern "C"
JNIEXPORT void JNICALL Java_org_mattvchandler_a2050_MainActivity_clear( JNIEnv *env, jobject)
{
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}
