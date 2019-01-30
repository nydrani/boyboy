#ifndef BBOYCORE_H
#define BBOYCORE_H 1

#include <jni.h>
#include <string>
#include <android/log.h>

#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>

#define  LOG_TAG    "libbboycore"

#define DEBUG true
#if DEBUG
    #define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
    #define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
    #define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
    #define  LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#else
    #define  LOGI(...)
    #define  LOGE(...)
    #define  LOGD(...)
    #define  LOGV(...)
#endif
#define  LOGA(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)


#define TIME_STEP 60
#define MS_PER_UPDATE (1.0f / TIME_STEP)
#define MAX_FRAME_SKIP 10
#define MOVING_AVERAGE_ALPHA 0.9f
#define BILLION 1000000000
#define BILLION_FLOAT 1000000000.0f
#define M_PI_FLOAT 3.14159265358979323846f
#define WORLD_SIZE 100

#define POS_ATTRIB 0

#define DOT_RADIUS 1.0f

struct EventItem {
    float x;
    float y;
};

bool checkGLError(const char *funcName);
void printGLErrors();
void printGLString(const char *name, GLenum s);
struct EventItem convertScreenCoordToWorldCoord(struct EventItem &position);
struct EventItem convertWorldCoordToScreenCoord(struct EventItem &position);
GLuint createShader(GLenum shaderType, const char *src);
GLuint createProgram(const char *vtxSrc, const char *fragSrc);


extern "C" {
    JNIEXPORT jint JNI_OnLoad(JavaVM *, void *);
    JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_init(JNIEnv *, jclass);
    JNIEXPORT jboolean JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_initOpenGL(JNIEnv *, jclass);
    JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_setup(JNIEnv *, jclass, jint, jint);
    JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_run(JNIEnv *, jclass);
    JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_render(JNIEnv *, jclass);
    JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_resume(JNIEnv *, jclass);
    JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_pause(JNIEnv *, jclass);
    JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_resumeEngine(JNIEnv *, jclass);
    JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_pauseEngine(JNIEnv *, jclass);
    JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_shutdown(JNIEnv *, jclass);
    JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_obtainFPS(JNIEnv *, jclass, jobject);
    JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_obtainPos(JNIEnv *, jclass, jobject);
    JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_sendEvent(JNIEnv *, jclass, jobject);
}

#endif
