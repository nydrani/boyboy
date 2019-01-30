package xyz.velvetmilk.boyboyemulator

import android.util.Log

/**
 * @author Victor Zhang
 */
class BBoyGameEngineInterface {
    private lateinit var fpsRunner: BBoyFPSRunner
    private lateinit var posRunner: BBoyPosRunner
    private var hasStarted: Boolean = false

    companion object {
        private val TAG = BBoyGameEngineInterface::class.java.simpleName
    }


    fun initFPSRunner(fpsInfo: BBoyFPS, fpsUpdateListener: BBoyFPSRunner.OnFPSUpdateListener) {
        if (::fpsRunner.isInitialized) {
            fpsRunner.shutdown()
        }

        fpsRunner = BBoyFPSRunner(fpsInfo, fpsUpdateListener)
        // @TODO possible after shutdown that fpsRunner instance is expired
        Thread(fpsRunner).start()
    }

    fun initPosRunner(posInfo: BBoyInputEvent, posUpdateListener: BBoyPosRunner.OnPosUpdateListener) {
        if (::posRunner.isInitialized) {
            posRunner.shutdown()
        }

        posRunner = BBoyPosRunner(posInfo, posUpdateListener)
        // @TODO possible after shutdown that posRunner instance is expired
        Thread(posRunner).start()
    }

    fun shutdownFPSRunner() {
        fpsRunner.shutdown()
    }

    fun shutdownPosRunner() {
        posRunner.shutdown()
    }

    fun initOpenGL() {
        if (!BBoyJNILib.initOpenGL()) {
            Log.w(TAG, "Failed to initialise OpenGL program!")
        }
        BBoyJNILib.printOpenGLInfo()
    }

    fun initGameLoop() {
        BBoyJNILib.init()
    }
    
    fun runGameLoop() {
        if (!hasStarted) {
            BBoyJNILib.run()
            hasStarted = true
        }
    }

    fun pauseGameLoop() {
        BBoyJNILib.pause()
    }

    fun resumeGameLoop() {
        BBoyJNILib.resume()
    }

    fun pauseGameEngine() {
        BBoyJNILib.pauseEngine()
    }

    fun resumeGameEngine() {
        BBoyJNILib.resumeEngine()
    }

    fun finishGameLoop() {
        BBoyJNILib.shutdown()
        hasStarted = false
    }

    fun setupViewport(width: Int, height: Int) {
        BBoyJNILib.setup(width, height)
    }

    fun drawFrame() {
        BBoyJNILib.render()
    }
}
