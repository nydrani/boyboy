#include <string>
#include <thread>
#include <queue>
#include <cmath>
#include <ctime>
#include <random>
#include <iostream>

#include <jni.h>

#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "tools/tools.hpp"

#include "shapes/Circle.hpp"
#include "shapes/Object.hpp"

#include "bboycore.hpp"



static void initProgram();
static bool initOpenGL();
static bool initOpenGLObjects();
static void runGameLoop();
static bool setupScreen(int, int);
static void processInput();
static void renderFrame(double);
static void updateTime();
static void updateGame();
static void stepGame();
static void pauseGame();
static void resumeGame();
static void shutdown();
static void storeEvent(struct EventItem);
static void drawTouchDot();


static auto boxVertices = {1.0f, 1.0f, 0.0f,
                           1.0f, -1.0f, 0.0f,
                           -1.0f, -1.0f, 0.0f,
                           -1.0f, 1.0f, 0.0f};


static auto triangleVertices = {sin(degToRads(240.0f)) * DOT_RADIUS, cos(degToRads(240.0f)) * DOT_RADIUS, 0.0f,
                                sin(degToRads(120.0f)) * DOT_RADIUS, cos(degToRads(120.0f)) * DOT_RADIUS, 0.0f,
                                sin(degToRads(0.0f)) * DOT_RADIUS, cos(degToRads(0.0f)) * DOT_RADIUS, 0.0f};

static auto vertexShader = "uniform mat4 uMVPMatrix;\n"
                "attribute vec4 vPosition;\n"
                "void main() {\n"
                "  gl_Position = uMVPMatrix * vPosition;\n"
                "}\n";

static auto fragmentShader =
        "precision mediump float;\n"
                "void main() {\n"
                "  gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
                "}\n";

void printGLString(const char *name, GLenum s) {
    auto *v = glGetString(s);
    LOGI("GL %s = %s\n", name, v);
}

bool checkGLError(const char *funcName) {
    GLint err = glGetError();

    if (err != GL_NO_ERROR) {
        LOGE("GL error after %s(): 0x%08x\n", funcName, err);
        return true;
    }

    return false;
}

void printGLErrors() {
    GLint err = glGetError();
    while (err != GL_NO_ERROR) {
        LOGE("GL error: 0x%08x\n", err);
        err = glGetError();
    }
}

GLuint createShader(GLenum shaderType, const char *src) {
    GLuint shader = glCreateShader(shaderType);
    if (!shader) {
        checkGLError("glCreateShader");
        return 0;
    }

    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
        GLint infoLogLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);

        if (infoLogLen > 0) {
            GLchar *infoLog = (GLchar *) malloc((size_t) infoLogLen);

            if (infoLog) {
                glGetShaderInfoLog(shader, infoLogLen, NULL, infoLog);
                LOGE("Could not compile %s shader:\n%s\n",
                     shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment",
                     infoLog);
                free(infoLog);
            }
        }

        glDeleteShader(shader);
        shader = 0;
    }

    return shader;
}

GLuint createProgram(const char *vtxSrc, const char *fragSrc) {
    GLuint vtxShader = 0;
    GLuint fragShader = 0;
    GLuint program = 0;
    GLint linked = GL_FALSE;

    vtxShader = createShader(GL_VERTEX_SHADER, vtxSrc);
    if (!vtxShader) {
        glDeleteShader(vtxShader);
        glDeleteShader(fragShader);
        return program;
    }
    fragShader = createShader(GL_FRAGMENT_SHADER, fragSrc);
    if (!fragShader) {
        glDeleteShader(vtxShader);
        glDeleteShader(fragShader);
        return program;
    }

    program = glCreateProgram();
    if (!program) {
        checkGLError("glCreateProgram");

        glDeleteShader(vtxShader);
        glDeleteShader(fragShader);
        return program;
    }

    glAttachShader(program, vtxShader);
    glAttachShader(program, fragShader);

    glBindAttribLocation(program, POS_ATTRIB, "vPosition");

    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &linked);

    if (!linked) {
        LOGE("Could not link program");
        GLint infoLogLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLen);

        if (infoLogLen) {
            GLchar* infoLog = (GLchar*) malloc((size_t) infoLogLen);
            if (infoLog) {
                glGetProgramInfoLog(program, infoLogLen, NULL, infoLog);
                LOGE("Could not link program:\n%s\n", infoLog);
                free(infoLog);
            }
        }

        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(vtxShader);
    glDeleteShader(fragShader);
    return program;
}

// ===== program start =====
static GLuint program;
static int yeeNum;
static float bgColor;

static struct timespec prevTimeUPS;
static struct timespec prevTimeFPS;
static bool paused;
static float colorUpdate;

static double fps;
static double ups;
static double true_ups;
static double lag;

static int screenWidth;
static int screenHeight;
static float screenRatioX;
static float screenRatioY;

static float worldWidth;
static float worldHeight;

static float dotRotation;

static struct EventItem curPosition;

static std::queue<struct EventItem> inputBuffer;

static std::thread gameLoop;
static bool running;

static GLuint triangleBuffer, rectangleBuffer;
static GLuint triangleVAO, rectangleVAO;
static GLint mvpMatrixLoc;

static std::mt19937 rng;

static Circle circle;
static Object originPoint;
static glm::vec3 circlePosition;
// =========================

struct EventItem convertScreenCoordToWorldCoord(struct EventItem &position) {
    float xPos = (position.x / screenRatioX) - (worldWidth / 2.0f);
    float yPos = (position.y / screenRatioY) - (worldHeight / 2.0f);

    // negative y ratio since screen is top to bottom while coordinates is bottom to top
    return { xPos, -yPos };
}

struct EventItem convertWorldCoordToScreenCoord(struct EventItem &position) {
    float xPos = (position.x + (worldWidth / 2.0f)) * screenRatioX;
    float yPos = (-position.y + (worldHeight / 2.0f)) * screenRatioY;

    // negative y ratio since screen is top to bottom while coordinates is bottom to top
    return { xPos, yPos };
}

static double getElapsedTime(struct timespec &prevTime, struct timespec &curTime) {
    // calculate elapsed time between prev and cur
    __kernel_long_t elapsedSec = curTime.tv_sec - prevTime.tv_sec;
    long elapsedNSec = curTime.tv_nsec - prevTime.tv_nsec;

    // subtraction carry
    if (prevTime.tv_nsec  > curTime.tv_nsec) {
        elapsedSec--;
        elapsedNSec = curTime.tv_nsec - prevTime.tv_nsec + BILLION;
    }

    // convert to double
    double elapsed = elapsedSec + elapsedNSec / BILLION_DOUBLE;

    return elapsed;
}

static void initProgram() {
    LOGI("initProgram");

    // initialise variables
    yeeNum = 0;
    bgColor = 0.0f;

    struct timespec res;
    clock_gettime(CLOCK_MONOTONIC, &res);
    prevTimeFPS = res;
    prevTimeUPS = res;

    colorUpdate = 0.02f;
    fps = 0.0;
    ups = 0.0;
    true_ups = 0.0;
    lag = 0.0;
    dotRotation = 0.0f;

    curPosition = { 0.0f, 0.0f };

    rng.seed(std::random_device()());

    running = true;
    paused = false;
}

static bool initOpenGL() {
    LOGI("initOpenGL");

    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    program = createProgram(vertexShader, fragmentShader);
    if (!program) {
        LOGE("Could not create program.");
        return false;
    }

    // setup face culling
    glEnable(GL_CULL_FACE);

    // opengl init buffers and bind data
    // format of vertex attribute (POS_ATTRIB)
    glVertexAttribFormat(POS_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0);
    checkGLError("glVertexAttribFormat");

    // generate buffers
    glGenBuffers(1, &triangleBuffer);
    glGenBuffers(1, &rectangleBuffer);
    glGenVertexArrays(1, &triangleVAO);
    glGenVertexArrays(1, &rectangleVAO);

    // bind triangle
    glBindVertexArray(triangleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, triangleBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(*triangleVertices.begin()) * triangleVertices.size(), triangleVertices.begin(), GL_STATIC_DRAW);
    glVertexAttribPointer(POS_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(POS_ATTRIB);

    // bind rectangle
    glBindVertexArray(rectangleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rectangleBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(*boxVertices.begin()) * boxVertices.size(), boxVertices.begin(), GL_STATIC_DRAW);
    glVertexAttribPointer(POS_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(POS_ATTRIB);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // setup program (shaders)
    glUseProgram(program);
    checkGLError("glUseProgram");

    mvpMatrixLoc = glGetUniformLocation(program, "uMVPMatrix");
    glUniformMatrix4fv(mvpMatrixLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1)));

    return true;
}

static bool initOpenGLObjects() {
    circle = Circle();
    originPoint = Object();
    originPoint.translation = glm::vec3(0, 10, 0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

static bool setupScreen(int w, int h) {
    LOGI("setupScreen(%d, %d)", w, h);

    screenWidth = w;
    screenHeight = h;

    // get ratio
    float aspectRatio = static_cast<float>(w) / static_cast<float>(h);
    if (aspectRatio >= 1.0f) {
        worldHeight = WORLD_SIZE / aspectRatio;
        worldWidth = WORLD_SIZE;
    } else {
        worldHeight = WORLD_SIZE;
        worldWidth = WORLD_SIZE * aspectRatio;
    }


    float convRatioY = screenHeight / worldHeight;
    float convRatioX = screenWidth / worldWidth;

    // check that the conversion ratios are almost equal
    // so that the world looks good
    assert(almostEquals(convRatioX, convRatioY));

    screenRatioX = convRatioX;
    screenRatioY = convRatioY;

    glViewport(0, 0, w, h);
    return !checkGLError("glViewport");
}

static void stepGame() {
    yeeNum++;
    yeeNum %= 60;

    // increase vs decrease color
    bgColor += colorUpdate;

    // toggle color
    if (bgColor > 1.0f) {
        colorUpdate = -0.02f;
    }

    if (bgColor < 0.0f) {
        colorUpdate = +0.02f;
    }

    // increase dot rotation
    dotRotation += 1.0f;
    if (dotRotation >= 360.0f) {
        dotRotation = 0.0f;
    }

    // update circle position
    if (yeeNum == 0) {
        std::uniform_real_distribution<float> rngPos(-20, 20);
        float x = rngPos(rng);
        float y = rngPos(rng);
        circlePosition = glm::vec3(x, y, 0);
    }
}

static void updateTime() {
    // update previous time to current time
    struct timespec res;
    clock_gettime(CLOCK_MONOTONIC, &res);

    // store current time
    prevTimeUPS = res;
}

static void updateGame() {
    // calculate time elapsed from previous update
    struct timespec res;
    clock_gettime(CLOCK_MONOTONIC, &res);
    double elapsed = getElapsedTime(prevTimeUPS, res);
    lag += elapsed;

    // num updates per second
    int updateCounter = 0;

    // update in steps
    while (lag >= MS_PER_UPDATE && updateCounter < MAX_FRAME_SKIP) {
        stepGame();
        lag -= MS_PER_UPDATE;

        updateCounter++;
    }

    if (updateCounter > 0) {
        ups = MOVING_AVERAGE_ALPHA * ups + (1.0 - MOVING_AVERAGE_ALPHA) * updateCounter / elapsed;
        LOGD("UPS: %f", ups);

        true_ups = MOVING_AVERAGE_ALPHA * true_ups + (1.0 - MOVING_AVERAGE_ALPHA) / elapsed;
        LOGD("TRUE_UPS: %f", true_ups);
    }
}

static void pauseGame() {
    paused = true;
}

static void resumeGame() {
    paused = false;
}

static void storeEvent(struct EventItem event) {
    // convert android xy coords to world coords
    struct EventItem convertedEvent = convertScreenCoordToWorldCoord(event);
    inputBuffer.push(convertedEvent);
}

static void processInput() {
    if (inputBuffer.empty()) {
        return;
    }

    struct EventItem s = inputBuffer.front();
    inputBuffer.pop();

    // ignore input if paused
    if (paused) {
        return;
    }

    curPosition.x = s.x;
    curPosition.y = s.y;
}

static void renderFrame(double interpolation) {
    // update fps average counter
    // obtain time elapsed for fps
    struct timespec res;
    clock_gettime(CLOCK_MONOTONIC, &res);
    double elapsed = getElapsedTime(prevTimeFPS, res);

    // store current times
    prevTimeFPS = res;

    // calculate fps here
    fps = MOVING_AVERAGE_ALPHA * fps + (1.0 - MOVING_AVERAGE_ALPHA) / elapsed;

    LOGD("FPS: %f", fps);


    // interpolate bgColor
    GLfloat interpBgColor = bgColor + colorUpdate * (float)interpolation;

    glClearColor(interpBgColor, interpBgColor, interpBgColor, 1.0f);
    checkGLError("glClearColor");

    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    checkGLError("glClear");

    // draw dot
    drawTouchDot();
}

static void drawTouchDot() {
    // place ortho camera to bottom left as 0,0
    // glm::mat4 orthoMat = glm::ortho(0.0f, (float)width, 0.0f, (float)height);

    // place ortho camera to 0,0 centre with world height and width with 16:9 ratio --> 160:90
    // Scale of screen : 1unit = 1metre
    // Depending on aspect ratio (smallest width is total 50)
    //
    //       +50
    //        |                            +25
    //        |                             |
    //  -25 --+-- +25      OR      -50 -----+----- +50
    //        |                             |
    //        |                            -25
    //       -50

    glm::mat4 orthoMat = glm::ortho(-worldWidth/2.0f, worldWidth/2.0f, -worldHeight/2.0f, worldHeight/2.0f);
    glm::mat4 modelMat, mat;

    modelMat = glm::mat4(1.0f);
    mat = orthoMat * modelMat;
    glUniformMatrix4fv(mvpMatrixLoc, 1, GL_FALSE, glm::value_ptr(mat));

    // draw rectangle loop at (0, 0)
    glBindVertexArray(rectangleVAO);
    glDrawArrays(GL_LINE_LOOP, 0, 4);

    // draw circle at random point between (-50, 50)
    modelMat = glm::translate(glm::mat4(1), circlePosition);
    mat = orthoMat * modelMat;
    glUniformMatrix4fv(mvpMatrixLoc, 1, GL_FALSE, glm::value_ptr(mat));
    circle.Draw();

    // translate the triangle to move it
    modelMat = glm::translate(glm::mat4(1), glm::vec3(curPosition.x, curPosition.y, 0));
    mat = orthoMat * modelMat;
    glUniformMatrix4fv(mvpMatrixLoc, 1, GL_FALSE, glm::value_ptr(mat));

    // draw triangle at current finger position offset from (0, 0)
    glBindVertexArray(triangleVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // draw objects with a scene graph (origin point at 10.0f on y axis)
    modelMat = glm::translate(glm::mat4(1), originPoint.translation);
    modelMat = glm::rotate(modelMat, degToRads(dotRotation), glm::vec3(0, 0, 1));
    mat = orthoMat * modelMat;
    glUniformMatrix4fv(mvpMatrixLoc, 1, GL_FALSE, glm::value_ptr(mat));
    originPoint.Draw();


    glBindVertexArray(0);
}

static void runGameLoop() {
    while (running) {
        processInput();

        if (!paused) {
            updateGame();
        }

        // rendering is externally called
        updateTime();

        // print any errors which may have occurred
        printGLErrors();
    }
}

static void shutdown() {
    running = false;
    gameLoop.join();

    // @TODO kill program shaders etc
    glDeleteBuffers(1, &rectangleBuffer);
    glDeleteBuffers(1, &triangleBuffer);
    glDeleteVertexArrays(1, &triangleVAO);
    glDeleteVertexArrays(1, &rectangleVAO);
}

extern "C"
{
JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    LOGV(__FUNCTION__, "onLoad");

    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }

    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_init(JNIEnv *env,
                                                                          jclass obj) {
    LOGV(__FUNCTION__, "init");

    initProgram();
}

JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_run(JNIEnv *env,
                                                                         jclass obj) {
    LOGV(__FUNCTION__, "init");

    running = true;
    gameLoop = std::thread(runGameLoop);
}

JNIEXPORT jboolean JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_initOpenGL(JNIEnv *env,
                                                                                    jclass obj) {
    LOGV(__FUNCTION__, "initOpenGL");

    bool success = initOpenGL();
    initOpenGLObjects();

    std::string hello = "initOpenGL";
    return jboolean(success);
}

JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_setup(JNIEnv *env,
                                                                           jclass obj,
                                                                           jint width,
                                                                           jint height) {
    LOGV(__FUNCTION__, "setup");

    setupScreen(width, height);
}

JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_render(JNIEnv *env,
                                                                            jclass obj) {
    LOGV(__FUNCTION__, "render");

    // interpolate the frame rendered between current and next [0-1]
    renderFrame(lag / MS_PER_UPDATE);
}

JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_pause(JNIEnv *env,
                                                                           jclass obj) {
    LOGV(__FUNCTION__, "pause");

    pauseGame();
}

JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_resume(JNIEnv *env,
                                                                            jclass obj) {
    LOGV(__FUNCTION__, "resume");

    resumeGame();
}

JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_shutdown(JNIEnv *env,
                                                                              jclass obj) {
    LOGV(__FUNCTION__, "shutdown");

    shutdown();
}

JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_obtainFPS(JNIEnv *env,
                                                                               jclass javaThis,
                                                                               jobject obj) {
    LOGV(__FUNCTION__, "obtainFPS");

    jclass clazz = env->GetObjectClass(obj);

    // Get Field references
    jfieldID param1Field = env->GetFieldID(clazz, "fps", "D");
    jfieldID param2Field = env->GetFieldID(clazz, "ups", "D");
    jfieldID param3Field = env->GetFieldID(clazz, "true_ups", "D");


    // Set fields for object
    env->SetDoubleField(obj, param1Field, fps);
    env->SetDoubleField(obj, param2Field, ups);
    env->SetDoubleField(obj, param3Field, true_ups);
}

JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_obtainPos(JNIEnv *env,
                                                                               jclass javaThis,
                                                                               jobject obj) {
    LOGV(__FUNCTION__, "obtainFPS");

    jclass clazz = env->GetObjectClass(obj);

    // Get Field references
    jfieldID param1Field = env->GetFieldID(clazz, "x", "F");
    jfieldID param2Field = env->GetFieldID(clazz, "y", "F");
    jfieldID param3Field = env->GetFieldID(clazz, "normX", "F");
    jfieldID param4Field = env->GetFieldID(clazz, "normY", "F");


    // Set fields for object
    struct EventItem converted = convertWorldCoordToScreenCoord(curPosition);
    env->SetFloatField(obj, param1Field, converted.x);
    env->SetFloatField(obj, param2Field, converted.y);

    // convert to normalised position
    env->SetFloatField(obj, param3Field, curPosition.x);
    env->SetFloatField(obj, param4Field, curPosition.y);
}

JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_sendEvent(JNIEnv *env,
                                                                               jclass javaThis,
                                                                               jobject obj) {
    LOGV(__FUNCTION__, "sendEvent");

    jclass clazz = env->GetObjectClass(obj);

    // Get Field references
    jfieldID param1Field = env->GetFieldID(clazz, "x", "F");
    jfieldID param2Field = env->GetFieldID(clazz, "y", "F");

    // Set fields for object
    jfloat x = env->GetFloatField(obj, param1Field);
    jfloat y = env->GetFloatField(obj, param2Field);

    // convert jobject to struct EventItem
    struct EventItem item = {x, y};

    storeEvent(item);
    // object class is a MotionEvent
    // store event into input buffer
}
}
