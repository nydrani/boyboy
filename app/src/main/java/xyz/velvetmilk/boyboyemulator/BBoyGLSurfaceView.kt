package xyz.velvetmilk.boyboyemulator

import android.content.Context
import android.opengl.EGL14
import android.opengl.GLSurfaceView
import android.util.AttributeSet
import android.util.Log
import android.view.MotionEvent
import android.view.View
import javax.microedition.khronos.egl.EGL10
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.egl.EGLContext
import javax.microedition.khronos.egl.EGLDisplay
import javax.microedition.khronos.opengles.GL10

/**
 * @author Victor Zhang
 */
class BBoyGLSurfaceView: GLSurfaceView {
    private val renderer = BBoyGLSurfaceRenderer()
    private val factory = BBoyEGLContextFactory()
    private val configChooser = BBoyEGLConfigChooser()

    private var mPreviousX: Float = 0f
    private var mPreviousY: Float = 0f

    private var touchdownX: Float = 0f
    private var touchdownY: Float = 0f


    constructor(context: Context): super(context)
    constructor(context: Context, attributeSet: AttributeSet): super(context, attributeSet)

    init {
        debugFlags = DEBUG_LOG_GL_CALLS or DEBUG_CHECK_GL_ERROR
        setEGLConfigChooser(configChooser)
        setEGLContextFactory(factory)
        setRenderer(renderer)
        renderMode = RENDERMODE_CONTINUOUSLY
    }

    companion object {
        private val TAG = BBoyGLSurfaceView::class.java.simpleName
        private const val TOUCH_SCALE_FACTOR: Float = 180.0f / 320f

        private fun checkEglError(prompt: String) {
            var error: Int

            do {
                error = EGL14.eglGetError()

                if (error != EGL14.EGL_SUCCESS) {
                    Log.e(TAG, "%s:  EGL_ERROR: 0x%x".format(prompt, error))
                }
            } while (error != EGL14.EGL_SUCCESS)
        }
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        super.onTouchEvent(event)

        val x: Float = event.x
        val y: Float = event.y

        BBoyJNILib.sendEvent(BBoyInputEvent(x, y))

        when (event.action) {
            MotionEvent.ACTION_DOWN -> {
                performClick()

                touchdownX = event.x
                touchdownY = event.y

                return true
            }
            MotionEvent.ACTION_MOVE -> {
                var dx: Float = x - mPreviousX
                var dy: Float = y - mPreviousY

                // reverse direction of rotation above the mid-line
                if (y > height / 2) {
                    dx *= -1
                }

                // reverse direction of rotation to left of the mid-line
                if (x < width / 2) {
                    dy *= -1
                }

                renderer.angle += (dx + dy) * TOUCH_SCALE_FACTOR

                mPreviousX = x
                mPreviousY = y
                return true
            }
        }

        return false
    }

    // Because we call this from onTouchEvent, this code will be executed for both
    // normal touch events and for when the system calls this using Accessibility
    override fun performClick(): Boolean {
        super.performClick()

        return true
    }

    class BBoyGLSurfaceRenderer: GLSurfaceView.Renderer {
        private val gameEngine = BBoyServiceProvider.getInstance().gameEngine

        @Volatile
        var angle: Float = 0f

        override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
            gameEngine.initOpenGL()
        }

        override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
            gameEngine.setupViewport(width, height)
        }

        override fun onDrawFrame(gl: GL10?) {
            gameEngine.drawFrame()
        }
    }

    // @TODO learn how to get gl version before initialising?
    class BBoyEGLContextFactory: GLSurfaceView.EGLContextFactory {
        companion object {
            private val TAG = BBoyEGLContextFactory::class.java.simpleName
            private const val GL_VERSION = 3

        }

        override fun createContext(egl: EGL10?, display: EGLDisplay?, eglConfig: EGLConfig?): EGLContext {
            Log.d(TAG, "Creating OpenGL ES %d context".format(GL_VERSION))

            val string = egl?.eglQueryString(display, EGL14.EGL_VERSION)
            Log.d(TAG, "EGL Version: %s".format(string))

            val attribList = intArrayOf(EGL14.EGL_CONTEXT_CLIENT_VERSION, GL_VERSION, EGL14.EGL_NONE)

            checkEglError("Before eglCreateContext")
            val context = egl?.eglCreateContext(display, eglConfig, EGL10.EGL_NO_CONTEXT, attribList)
            checkEglError("After eglCreateContext")

            return context!!
        }

        override fun destroyContext(egl: EGL10?, display: EGLDisplay?, context: EGLContext?) {
            egl?.eglDestroyContext(display, context)
        }
    }

    class BBoyEGLConfigChooser: GLSurfaceView.EGLConfigChooser {
        private val attribList = intArrayOf(
                EGL14.EGL_RED_SIZE, 4,
                EGL14.EGL_GREEN_SIZE, 4,
                EGL14.EGL_BLUE_SIZE, 4,
                EGL14.EGL_RENDERABLE_TYPE, EGL14.EGL_OPENGL_ES2_BIT,
                EGL14.EGL_NONE)

        override fun chooseConfig(egl: EGL10?, display: EGLDisplay?): EGLConfig {
            /* Get the number of minimally matching EGL configurations
             */
            val numConfig = IntArray(1)
            egl?.eglChooseConfig(display, attribList, null, 0, numConfig)

            // Check that a config matches
            val numConfigs = numConfig[0]
            if (numConfigs <= 0) {
                throw IllegalArgumentException("No configs match configSpec")
            }

            /* Allocate then read the array of minimally matching EGL configs
             */
            val configs = arrayOfNulls<EGLConfig>(numConfigs)
            egl?.eglChooseConfig(display, attribList, configs, numConfigs, numConfig)

            /* Now return the "best" one
             */
            return configs[0]!!
        }
    }
}
