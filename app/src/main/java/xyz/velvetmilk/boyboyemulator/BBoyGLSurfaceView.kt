package xyz.velvetmilk.boyboyemulator

import android.content.Context
import android.opengl.EGL14
import android.opengl.EGLExt
import android.opengl.GLES10
import android.opengl.GLSurfaceView
import android.util.AttributeSet
import android.util.Log
import android.view.MotionEvent
import javax.microedition.khronos.egl.EGL10
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.egl.EGLContext
import javax.microedition.khronos.egl.EGLDisplay
import javax.microedition.khronos.opengles.GL10
import kotlin.math.absoluteValue
import kotlin.math.roundToInt

/**
 * @author Victor Zhang
 */
class BBoyGLSurfaceView: GLSurfaceView {
    private val renderer = BBoyGLSurfaceRenderer(object : BBoyGLSurfaceRenderer.SurfaceCreatedListener {
        override fun onSurfaceCreated() {
            openGLVersion = GLES10.glGetString(GLES10.GL_VERSION)
            openGLRenderer = GLES10.glGetString(GLES10.GL_RENDERER)
            openGLExtensions = GLES10.glGetString(GLES10.GL_EXTENSIONS)

            openGLReadyListener.onOpenGLReady()
        }
    })
    private val factory = BBoyEGLContextFactory()
    private val configChooser = BBoyEGLConfigChooser()

    private var openGLReadyListener: OpenGLReadyListener = object : OpenGLReadyListener {
        override fun onOpenGLReady() {
            Log.d(TAG, "onOpenGLReady")
        }
    }

    interface OpenGLReadyListener {
        fun onOpenGLReady()
    }

    lateinit var openGLVersion: String
    lateinit var openGLRenderer: String
    lateinit var openGLExtensions: String

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
        private const val MAX_TOUCH_POINTERS = 10

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

    fun addOpenGLReadyListener(listener: OpenGLReadyListener) {
        openGLReadyListener = listener
    }


    override fun onTouchEvent(event: MotionEvent): Boolean {
        super.onTouchEvent(event)

        // NOTE: multi-touch
        // problem is how to assign touchpointer id with which touch slot
        // right now (since touchevents are discrete and shouldnt interact with each other)
        // we can safely ignore which touch correlates to which pointer
        val touchers = if (event.pointerCount > MAX_TOUCH_POINTERS) {
            MAX_TOUCH_POINTERS
        } else {
            event.pointerCount
        }

        val xPrecision = event.xPrecision
        val yPrecision = event.yPrecision

        val touchList: MutableList<BBoyInputEvent> = mutableListOf()

        for (i in 0 until touchers) {
            var x: Float = event.getX(i) * xPrecision
            var y: Float = event.getY(i) * yPrecision

            // if (x is almost an int) --> use int version
            // @TODO fix epsilon to a better value than constant recalculation
            // @TODO change to multiple of ulp if required???
            // @TODO optmise if this is really slow
            val roundedX = x.roundToInt().toFloat()
            val roundedY = y.roundToInt().toFloat()

            var epsilon = Math.ulp(roundedX)
            if (roundedX != x && (roundedX - x).absoluteValue <= epsilon) {
                x = roundedX
            }

            epsilon = Math.ulp(roundedY)
            if (roundedY != y && (roundedY - y).absoluteValue <= epsilon) {
                y = roundedY
            }

            touchList.add(BBoyInputEvent(x, y))
        }

        //Log.d(TAG, "x: " + event.x * event.xPrecision + " | y: " + event.y * event.xPrecision)
//        BBoyJNILib.sendEvent(BBoyInputEvent(x, y))
        BBoyJNILib.sendEvent(touchList.toTypedArray())

        when (event.action and MotionEvent.ACTION_MASK) {
            MotionEvent.ACTION_DOWN -> {
                //Log.d(TAG, "x: " + event.x + " | y: " + event.y + " | motionevent: action_down")
                performClick()

                return true
            }
            MotionEvent.ACTION_MOVE -> {
                //Log.d(TAG, "x: " + event.x + " | y: " + event.y + " | motionevent: action_move")
                return true
            }
            MotionEvent.ACTION_UP -> {
                //Log.d(TAG, "x: " + event.x + " | y: " + event.y + " | motionevent: action_up")
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

    class BBoyGLSurfaceRenderer(listener: SurfaceCreatedListener): GLSurfaceView.Renderer {
        private val gameEngine = BBoyServiceProvider.getInstance().gameEngine
        private val surfaceCreatedListener: SurfaceCreatedListener = listener

        interface SurfaceCreatedListener {
            fun onSurfaceCreated()
        }

        override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
            gameEngine.initOpenGL()
            surfaceCreatedListener.onSurfaceCreated()
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
                EGL14.EGL_RENDERABLE_TYPE,
                EGLExt.EGL_OPENGL_ES3_BIT_KHR,
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
