#define GLM_ENABLE_EXPERIMENTAL

#include <string>
#include <thread>
#include <queue>
#include <cmath>
#include <ctime>
#include <random>
#include <memory>
#include <iostream>

#include <jni.h>

#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <set>
#include <sstream>

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
static void renderFrame();
static void updateTime();
static void updateGame();
static void stepGame();
static void pauseGame();
static void resumeGame();
static void pauseGameEngine();
static void resumeGameEngine();
static void shutdown();
static void storeEvent(std::vector<struct EventItem> const&);
static void drawTouchDot();


static auto boxVertices = {1.0f, 1.0f, 0.0f,
                           1.0f, -1.0f, 0.0f,
                           -1.0f, -1.0f, 0.0f,
                           -1.0f, 1.0f, 0.0f};

static auto vertexShader =
        "uniform mat4 uMVPMatrix;\n"
        "attribute vec4 vPosition;\n"
        "void main() {\n"
        "  gl_Position = uMVPMatrix * vPosition;\n"
        "}\n";

static auto fragmentShader =
        "uniform vec4 color;\n"
        "void main() {\n"
        "  gl_FragColor = color;\n"
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

    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
        GLint infoLogLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);

        if (infoLogLen > 0) {
            auto* infoLog = (GLchar*) malloc((size_t) infoLogLen);

            if (infoLog) {
                glGetShaderInfoLog(shader, infoLogLen, nullptr, infoLog);
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
            auto* infoLog = (GLchar*) malloc((size_t) infoLogLen);
            if (infoLog) {
                glGetProgramInfoLog(program, infoLogLen, nullptr, infoLog);
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
static struct timespec prevTimeSPS;
static struct timespec prevTimeFPS;
static struct timespec timeDiff;
static bool paused;
static bool enginePaused;
static float colorUpdate;

static float sps;
static float fps;
static float ups;
static float true_ups;
static float lag;
static float interpolation;

static int screenWidth;
static int screenHeight;

static float worldWidth;
static float worldHeight;

// NOTE: Probably remove current position list off to be NOT lingering
static std::vector<struct EventItem> curPositionList;
static std::vector<struct EventItem> rawPositionList;

static std::mutex pauseMutex;

static std::queue<std::vector<struct EventItem>> inputBuffer;
static std::queue<std::vector<struct EventItem>> rawInputBuffer;

static std::thread gameLoop;
static bool running;
static bool openGLReady;

static std::set<Object*> allGameObjects;
static std::set<Object*> rootGameObjects;

static struct timespec startTime;
static struct timespec curTime;

static uint64_t currentFrame;
static uint64_t currentSteppedFrame;

static GLint mvpMatrixLoc;
static GLint colorVecLoc;

static std::mt19937 rng;

static Circle circle;
static Object originPoint;
static Object* pointerList[MAX_POINTER_SIZE];

static Object puckObject;
static Object player1Object;
static Object player2Object;
// =========================

// normalization formula : x between [a, b]
// https://stats.stackexchange.com/questions/178626/how-to-normalize-data-between-1-and-1
void convertScreenCoordToWorldCoord(struct EventItem& position) {
    // need to -1 in width and height for 0th offset
    float xPos = worldWidth * position.x / (screenWidth - 1) - (worldWidth / 2);
    float yPos = worldHeight * position.y / (screenHeight - 1) - (worldHeight / 2);

    // negative y ratio since screen is top to bottom while coordinates is bottom to top
    position.x = xPos;
    position.y = -yPos;
}


struct EventItem convertScreenCoordToWorldCoord(struct EventItem const& position) {
    // need to -1 in width and height for 0th offset
    float xPos = worldWidth * position.x / (screenWidth - 1) - (worldWidth / 2);
    float yPos = worldHeight * position.y / (screenHeight - 1) - (worldHeight / 2);

    // negative y ratio since screen is top to bottom while coordinates is bottom to top
    return { xPos, -yPos };
}

void convertWorldCoordToScreenCoord(struct EventItem& position) {
    // need to -1 in width and height for 0th offset
    float xPos = (screenWidth - 1) * (position.x + worldWidth / 2) / worldWidth;
    float yPos = (screenHeight - 1) * (position.y + worldHeight / 2) / worldHeight;

    // negative y ratio since screen is top to bottom while coordinates is bottom to top
    position.x = xPos;
    position.y = -yPos;
}

struct EventItem convertWorldCoordToScreenCoord(struct EventItem const& position) {
    // need to -1 in width and height for 0th offset
    float xPos = (screenWidth - 1) * (position.x + worldWidth / 2) / worldWidth;
    float yPos = (screenHeight - 1) * (position.y + worldHeight / 2) / worldHeight;

    // negative y ratio since screen is top to bottom while coordinates is bottom to top
    return { xPos, -yPos };
}

static float getElapsedTime(struct timespec const& prevTime, struct timespec const& curTime) {
    // calculate elapsed time between prev and cur
    __kernel_long_t elapsedSec = curTime.tv_sec - prevTime.tv_sec;
    long elapsedNSec = curTime.tv_nsec - prevTime.tv_nsec;

    // subtraction carry
    if (prevTime.tv_nsec > curTime.tv_nsec) {
        --elapsedSec;
        elapsedNSec += BILLION;
    }

    // convert to float
    float elapsed = elapsedSec + elapsedNSec / BILLION_FLOAT;

    return elapsed;
}

static void initProgram() {
    LOGI("initProgram");

    // initialise variables
    yeeNum = 0;
    bgColor = 0.0f;

//    struct timespec res;
//    clock_gettime(CLOCK_MONOTONIC, &res);
//    prevTimeFPS = res;
//    prevTimeUPS = res;
//    prevTimeSPS = res;
//    curTime = res;
//    startTime = res;

    colorUpdate = 0.02f;

    sps = 0.0f;
    fps = 0.0f;
    ups = 0.0f;
    true_ups = 0.0f;
    lag = 0.0f;
    interpolation = 0.0f;

    currentFrame = 0;
    currentSteppedFrame = 0;

    clock_gettime(CLOCK_MONOTONIC, &curTime);
    clock_gettime(CLOCK_MONOTONIC, &startTime);

    rng.seed(std::random_device()());

    running = false;
    openGLReady = false;
    paused = false;
    enginePaused = false;
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
    checkGLError("createProgram");

    // setup face culling
    glEnable(GL_CULL_FACE);
    checkGLError("glEnable");

    // setup program (shaders)
    glUseProgram(program);
    checkGLError("glUseProgram");

    // get uniform variable location in GPU
    mvpMatrixLoc = glGetUniformLocation(program, "uMVPMatrix");
    colorVecLoc = glGetUniformLocation(program, "color");
    glUniformMatrix4fv(mvpMatrixLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1)));
    glUniform4fv(colorVecLoc, 1, glm::value_ptr(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)));

    openGLReady = true;

    return true;
}

// TODO decouple this from OPENGL renderer (so it doenst call shit)
static bool initOpenGLObjects() {
    circle = Circle();
    originPoint = Object();
    auto childObj = std::make_unique<Object>();

    for (auto& item : pointerList) {
        delete item;
        item = new Object();
        allGameObjects.emplace(item);
        rootGameObjects.emplace(item);
    }

    // create puck and two handles
    puckObject = Object();
    puckObject.velocity = glm::vec3(0.05f, 0.05f, 0.0f);
    player1Object = Object();
    player2Object = Object();

    // add to list of game objects
    allGameObjects.emplace(&puckObject);
    allGameObjects.emplace(&player1Object);
    allGameObjects.emplace(&player2Object);
    allGameObjects.emplace(&originPoint);
    allGameObjects.emplace(childObj.get());

    rootGameObjects.emplace(&puckObject);
    rootGameObjects.emplace(&player1Object);
    rootGameObjects.emplace(&player2Object);
    rootGameObjects.emplace(&originPoint);

    // modify objects
    originPoint.translation = glm::vec3(0, 10, 0);
    childObj->translation = glm::vec3(10, 0, 0);
    originPoint.addChild(std::move(childObj));

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

    glViewport(0, 0, w, h);
    return !checkGLError("glViewport");
}

static void stepGame() {
    currentSteppedFrame++;

    yeeNum++;
    yeeNum %= TIME_STEP;

    // increase vs decrease color
    bgColor += colorUpdate;

    // toggle color
    if (bgColor > 1.0f) {
        colorUpdate = -1.0f * MS_PER_UPDATE / 2;
    }

    if (bgColor < 0.0f) {
        colorUpdate = +1.0f * MS_PER_UPDATE / 2;
    }

    // clamp bgcolor to [0, 1]
    bgColor = glm::clamp(bgColor, 0.0f, 1.0f);

    // update random position between [-20, 20]
//    if (yeeNum == 0) {
//        std::uniform_real_distribution<float> rngPos(-20, 20);
//        float x = rngPos(rng);
//        float y = rngPos(rng);
//    }

    // disable non updated pointers
    for (auto const& item : pointerList) {
        if (item == nullptr) {
            continue;
        }
        item->isActive = false;
    }

    // update the position of pointers
    for (int i = 0; i < curPositionList.size(); ++i) {
        if (pointerList[i] == nullptr) {
            continue;
        }
        pointerList[i]->translation = glm::vec3(curPositionList[i].x, curPositionList[i].y, 0.0f);
        pointerList[i]->isActive = true;
    }

    // update the rotation of the cool object
    originPoint.rotation = glm::rotate(originPoint.rotation, glm::radians(1.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    originPoint.rotation = glm::normalize(originPoint.rotation);

    // update TRS of puck and players
    player1Object.translation = glm::vec3(0.0f, -20.0f, 0.0f);
    player2Object.translation = glm::vec3(0.0f, 20.0f, 0.0f);

    std::set<Object*> collidedObjects;
    std::set<Object*> nonCollidedObjects;

    // update objects
    for (auto const& it : rootGameObjects) {
        it->Update();
    }

    // custom code for puck updating
    if (puckObject.translation.x < -worldWidth / 2 || puckObject.translation.x > worldWidth / 2) {
        puckObject.velocity.x = -puckObject.velocity.x;
    }

    if (puckObject.translation.y < -worldHeight/ 2 || puckObject.translation.y > worldHeight / 2) {
        std::uniform_real_distribution<float> rngVel(-0.05f, 0.05f);
        float v = rngVel(rng);
        puckObject.velocity = glm::vec3(v, v, 0.0f);
        puckObject.translation = glm::vec3(5.0f, 0.0f, 0.0f);
    }

    // check collision
    for (auto const& it : allGameObjects) {
        // skip for inactive objects
        if (!it->isActive) {
            continue;
        }
        // find a list of objects which have collided
        for (auto const& other : allGameObjects) {
            // skip if me == other
            if (it == other) {
                continue;
            }

            // skip for inactive objects
            if (!other->isActive) {
                continue;
            }

            if (it->checkCollision(*other)) {
                // load a list of collided objects
                collidedObjects.emplace(it);
                collidedObjects.emplace(other);
            }
        }
    }

    // set collided objects to a yellow color
    for (auto const& it : collidedObjects) {
        if (it == &puckObject) {
            it->velocity.y = -(it->velocity.y * 1.1f);
            it->velocity.x = -(it->velocity.x * 1.1f);
        }
        it->color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    }

    std::set_difference(allGameObjects.begin(), allGameObjects.end(), collidedObjects.begin(), collidedObjects.end(), std::inserter(nonCollidedObjects, nonCollidedObjects.end()));
    for (auto const& it : nonCollidedObjects) {
        it->color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    }
}

static void updateTime() {
    // update previous time to current time
    struct timespec res;
    clock_gettime(CLOCK_MONOTONIC, &res);

    // store current time
    curTime = res;
    prevTimeUPS = res;
}

static void updateGame() {
    // calculate time elapsed from previous update
    struct timespec res;
    clock_gettime(CLOCK_MONOTONIC, &res);
    float elapsed = getElapsedTime(prevTimeUPS, res);
    lag += elapsed;

    // update time before stepping game
    curTime = res;
    prevTimeUPS = res;

    // num steps per update call
    int stepCounter = 0;

    // update in steps
    while (lag >= MS_PER_UPDATE && stepCounter < MAX_FRAME_SKIP) {
        if (!paused) {
            stepGame();
        }
        lag -= MS_PER_UPDATE;

        currentFrame++;
        stepCounter++;

//        // update steps per second counter
//        float elapsedSPS = getElapsedTime(prevTimeSPS, res);
//        if (elapsedSPS != 0.0f) {
//            sps = MOVING_AVERAGE_ALPHA * sps + (1.0f - MOVING_AVERAGE_ALPHA) / elapsedSPS;
//        }
//        prevTimeSPS = res;
    }

    if (!paused) {
        interpolation = lag / MS_PER_UPDATE;
    }

    // update debug counters
    true_ups = MOVING_AVERAGE_ALPHA * true_ups + (1.0f - MOVING_AVERAGE_ALPHA) / elapsed;

    if (stepCounter > 0) {
        ups = MOVING_AVERAGE_ALPHA * ups + (1.0f - MOVING_AVERAGE_ALPHA) * stepCounter / elapsed;

        float elapsedSPS = getElapsedTime(prevTimeSPS, res);
        sps = MOVING_AVERAGE_ALPHA * sps + (1.0f - MOVING_AVERAGE_ALPHA) * stepCounter / elapsedSPS;
        prevTimeSPS = res;
    }
}

static void pauseGame() {
    paused = true;
}

static void resumeGame() {
    paused = false;
}

static void pauseGameEngine() {
    // dont attempt to grab mutex again (from android UI thread)
    if (enginePaused) {
        return;
    }

    // set flag before attempting to lock
    enginePaused = true;
    pauseMutex.lock();

    struct timespec res;
    clock_gettime(CLOCK_MONOTONIC, &res);

    // store time difference
    timeDiff.tv_sec = res.tv_sec - prevTimeUPS.tv_sec;
    timeDiff.tv_nsec = res.tv_nsec - prevTimeUPS.tv_nsec;
}

static void resumeGameEngine() {
    // set flag only after unlock attempt
    pauseMutex.unlock();
    enginePaused = false;

    // restore time difference
    struct timespec res;
    clock_gettime(CLOCK_MONOTONIC, &res);

    prevTimeUPS.tv_sec = res.tv_sec - timeDiff.tv_sec;
    prevTimeUPS.tv_nsec = res.tv_nsec - timeDiff.tv_nsec;
}

static void storeEvent(std::vector<struct EventItem> const& event) {
    // convert android xy coords to world coords
    std::vector<struct EventItem> convertedList;
    for (auto& item : event) {
        struct EventItem convertedEvent = convertScreenCoordToWorldCoord(item);
        convertedList.emplace_back(convertedEvent);
    }

    rawInputBuffer.push(event);
    inputBuffer.push(convertedList);
}

static void processInput() {
    if (inputBuffer.empty()) {
        return;
    }

    std::vector<struct EventItem> s = inputBuffer.front();
    inputBuffer.pop();

    // ignore input if paused
    if (paused) {
        return;
    }

    curPositionList = s;
}

static void processRawInput() {
    if (rawInputBuffer.empty()) {
        return;
    }

    std::vector<struct EventItem> s = rawInputBuffer.front();
    rawInputBuffer.pop();

    // ignore input if paused
    if (paused) {
        return;
    }

    rawPositionList = s;
}

static void renderFrame() {
    // update fps average counter
    // obtain time elapsed for fps
    struct timespec res;
    clock_gettime(CLOCK_MONOTONIC, &res);
    float elapsed = getElapsedTime(prevTimeFPS, res);

    // store current times
    prevTimeFPS = res;

    // calculate fps here
    fps = MOVING_AVERAGE_ALPHA * fps + (1.0f - MOVING_AVERAGE_ALPHA) / elapsed;

    // interpolate bgColor
    GLfloat interpBgColor = bgColor + colorUpdate * interpolation;

    glClearColor(interpBgColor, interpBgColor, interpBgColor, 1.0f);
    checkGLError("glClearColor");

    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    checkGLError("glClear");

    // draw dot
    drawTouchDot();
}

// @TODO convert to renderer with interpolation
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

    // draw circle at (0, 0)
    modelMat = glm::mat4(1.0f);
    mat = orthoMat * modelMat;
    glUniformMatrix4fv(mvpMatrixLoc, 1, GL_FALSE, glm::value_ptr(mat));
    glUniform4fv(colorVecLoc, 1, glm::value_ptr(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)));

    // draw random circle
    circle.Draw();

    // translate the triangle to move it
    modelMat = glm::mat4(1.0f);
    mat = orthoMat * modelMat;

    // draw all gameobjects
    for (auto const& it : allGameObjects) {
        // skip for non root objects
        if (it->parent != nullptr) {
            continue;
        }

        // skip for inactive objects
        if (!it->isActive) {
            continue;
        }

        // draw
        it->Draw(mat, mvpMatrixLoc, colorVecLoc);
    }

    // draw aabb
    for (auto const& it : allGameObjects) {
        // skip for non root objects
        if (it->parent != nullptr) {
            continue;
        }

        // skip for inactive objects
        if (!it->isActive) {
            continue;
        }

        // draw AABB
        it->DrawAABB(mat, mvpMatrixLoc, colorVecLoc);
    }

//    // draw objects with a scene graph (origin point at 10.0f on y axis)
//    originPoint.Draw(mat, mvpMatrixLoc, colorVecLoc);
//    originPoint.DrawAABB(mat, mvpMatrixLoc, colorVecLoc);
//
//    for (int i = 0; i < curPositionList.size(); ++i) {
//        pointerList[i]->Draw(mat, mvpMatrixLoc, colorVecLoc);
//        pointerList[i]->DrawAABB(mat, mvpMatrixLoc, colorVecLoc);
//    }

    glBindVertexArray(0);
}

static void runGameLoop() {
    // setup time based variables
    struct timespec res;
    clock_gettime(CLOCK_MONOTONIC, &res);
    prevTimeFPS = res;
    prevTimeUPS = res;
    prevTimeSPS = res;
    curTime = res;
    startTime = res;


    while (running) {
        if (!openGLReady) {
            continue;
        }
        // call mutex conditions to pause thread when game is closed
        std::lock_guard<std::mutex> lock(pauseMutex);


        // @TODO check if there is an issue here when game runs slower than render
        processInput();
        processRawInput();

        updateGame();

        // rendering is externally called
//        updateTime();

        // print any errors which may have occurred
        printGLErrors();

    }
}

static void shutdown() {
    running = false;
    gameLoop.join();


    // @TODO kill program shaders etc (may not be necessary since OS just cleans this up)
    glDeleteProgram(program);
}

extern "C" {
JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    LOGV(__FUNCTION__, "onLoad");

    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    return JNI_VERSION_1_6;
}

JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {
    LOGV(__FUNCTION__, "onUnload");

    // do nothing lol
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

    printCurrentThread("opengl init");

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
    renderFrame();
}

JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_resume(JNIEnv *env,
                                                                           jclass obj) {
    LOGV(__FUNCTION__, "resume");

    resumeGame();
}

JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_pause(JNIEnv *env,
                                                                           jclass obj) {
    LOGV(__FUNCTION__, "pause");

    pauseGame();
}

JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_resumeEngine(JNIEnv *env,
                                                                                  jclass obj) {
    LOGV(__FUNCTION__, "resumeGameEngine");

    printCurrentThread("resumeGameEngine");

    resumeGameEngine();
}

JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_pauseEngine(JNIEnv *env,
                                                                           jclass obj) {
    LOGV(__FUNCTION__, "pauseGameEngine");

    printCurrentThread("pauseGameEngine");

    pauseGameEngine();
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
    jfieldID param1Field = env->GetFieldID(clazz, "fps", "F");
    jfieldID param2Field = env->GetFieldID(clazz, "ups", "F");
    jfieldID param3Field = env->GetFieldID(clazz, "true_ups", "F");
    jfieldID param4Field = env->GetFieldID(clazz, "frame", "J");
    jfieldID param5Field = env->GetFieldID(clazz, "stepped_frame", "J");
    jfieldID param6Field = env->GetFieldID(clazz, "cur_time", "J");
    jfieldID param7Field = env->GetFieldID(clazz, "sps", "F");

    // Set fields for object
    env->SetFloatField(obj, param1Field, fps);
    env->SetFloatField(obj, param2Field, ups);
    env->SetFloatField(obj, param3Field, true_ups);
    env->SetLongField(obj, param4Field, currentFrame);
    env->SetLongField(obj, param5Field, currentSteppedFrame);
    env->SetLongField(obj, param6Field, curTime.tv_sec - startTime.tv_sec);
    env->SetFloatField(obj, param7Field, sps);
}

JNIEXPORT jobjectArray JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_obtainPos(JNIEnv *env,
                                                                               jclass javaThis) {
    LOGV(__FUNCTION__, "obtainFPS");

    unsigned long positionListSize = curPositionList.size();

    jclass clazz = env->FindClass("xyz/velvetmilk/boyboyemulator/BBoyInputEvent");
    jmethodID constructor = env->GetMethodID(clazz, "<init>", "()V");
    jobjectArray retArray = env->NewObjectArray(static_cast<jsize>(positionListSize), clazz, nullptr);

    for (int i = 0; i < positionListSize; ++i) {
        jobject newObj = env->NewObject(clazz, constructor);

        // Get Field references
        jfieldID param1Field = env->GetFieldID(clazz, "x", "F");
        jfieldID param2Field = env->GetFieldID(clazz, "y", "F");
        jfieldID param3Field = env->GetFieldID(clazz, "normX", "F");
        jfieldID param4Field = env->GetFieldID(clazz, "normY", "F");

        // Set fields for object
        env->SetFloatField(newObj, param1Field, rawPositionList[i].x);
        env->SetFloatField(newObj, param2Field, rawPositionList[i].y);

//    struct EventItem converted = convertWorldCoordToScreenCoord(curPosition);
//    env->SetFloatField(obj, param1Field, converted.x);
//    env->SetFloatField(obj, param2Field, converted.y);

        env->SetFloatField(newObj, param3Field, curPositionList[i].x);
        env->SetFloatField(newObj, param4Field, curPositionList[i].y);

        env->SetObjectArrayElement(retArray, i, newObj);
    }

    return retArray;
}

JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_obtainPosInplace(JNIEnv *env,
                                                                                       jclass javaThis,
                                                                                       jobjectArray objArray) {
    LOGV(__FUNCTION__, "obtainFPSInplace");

    unsigned long positionListSize = curPositionList.size();

    jclass clazz = env->FindClass("xyz/velvetmilk/boyboyemulator/BBoyInputEvent");

    for (int i = 0; i < positionListSize; ++i) {

        // Get Field references
        jfieldID param1Field = env->GetFieldID(clazz, "x", "F");
        jfieldID param2Field = env->GetFieldID(clazz, "y", "F");
        jfieldID param3Field = env->GetFieldID(clazz, "normX", "F");
        jfieldID param4Field = env->GetFieldID(clazz, "normY", "F");

        // load memory already provided
        jobject curElement = env->GetObjectArrayElement(objArray, i);
        env->SetFloatField(curElement, param1Field, rawPositionList[i].x);
        env->SetFloatField(curElement, param2Field, rawPositionList[i].y);
        env->SetFloatField(curElement, param3Field, curPositionList[i].x);
        env->SetFloatField(curElement, param4Field, curPositionList[i].y);
        env->SetObjectArrayElement(objArray, i, curElement);
    }
}

JNIEXPORT void JNICALL Java_xyz_velvetmilk_boyboyemulator_BBoyJNILib_sendEvent(JNIEnv *env,
                                                                               jclass javaThis,
                                                                               jobjectArray objArray) {
    LOGV(__FUNCTION__, "sendEvent");

    std::vector<struct EventItem> eventList;

    jsize length = env->GetArrayLength(objArray);
    for (int i = 0; i < length; ++i) {
        jobject obj = env->GetObjectArrayElement(objArray, i);
        jclass clazz = env->GetObjectClass(obj);

        // Get Field references
        jfieldID param1Field = env->GetFieldID(clazz, "x", "F");
        jfieldID param2Field = env->GetFieldID(clazz, "y", "F");

        // Set fields for object
        jfloat x = env->GetFloatField(obj, param1Field);
        jfloat y = env->GetFloatField(obj, param2Field);

        // convert jobject to struct EventItem
        eventList.emplace_back(x, y);
    }
    // object class is a MotionEvent
    // store event into input buffer
    storeEvent(eventList);
}
}
