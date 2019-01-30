package xyz.velvetmilk.boyboyemulator

import android.opengl.GLES10
import android.opengl.GLES32
import android.util.Log
import android.view.MotionEvent
import java.nio.IntBuffer
import javax.microedition.khronos.opengles.GL10

/**
 * @author Victor Zhang
 */
class BBoyJNILib {

    companion object {
        private val TAG = BBoyJNILib::class.java.simpleName

        init {
            System.loadLibrary("bboycore")
        }

        @JvmStatic
        external fun init()

        @JvmStatic
        external fun initOpenGL(): Boolean

        @JvmStatic
        external fun setup(width: Int, height: Int)

        @JvmStatic
        external fun run()

        @JvmStatic
        external fun render()

        @JvmStatic
        external fun pause()

        @JvmStatic
        external fun resume()

        @JvmStatic
        external fun pauseEngine()

        @JvmStatic
        external fun resumeEngine()

        @JvmStatic
        external fun shutdown()

        @JvmStatic
        external fun obtainFPS(fpsInfo: BBoyFPS)

        @JvmStatic
        external fun obtainPos(posInfo: BBoyInputEvent)

        @JvmStatic
        external fun sendEvent(event: BBoyInputEvent)

        fun printOpenGLInfo() {
            val buffer = IntBuffer.allocate(1)

            val extensions = GLES10.glGetString(GLES10.GL_EXTENSIONS)
            val version = GLES10.glGetString(GLES10.GL_VERSION)
            val renderer = GLES10.glGetString(GLES10.GL_RENDERER)

            // @NOTE only for opengles 3.2 (api 24+)
//            GLES32.glGetIntegerv(GLES32.GL_MAJOR_VERSION, buffer)
//            val majorVersion = buffer[0]
//            GLES32.glGetIntegerv(GLES32.GL_MINOR_VERSION, buffer)
//            val minorVersion = buffer[0]

            val strBuilder = StringBuilder()
            strBuilder.append("Extensions: ")
            strBuilder.append(extensions)
            strBuilder.append("\n")
            strBuilder.append("Renderer: ")
            strBuilder.append(renderer)
            strBuilder.append("\n")
            strBuilder.append("Version: ")
            strBuilder.append(version)
            strBuilder.append("\n")
//            strBuilder.append("VersionCode: ")
//            strBuilder.append(majorVersion)
//            strBuilder.append(".")
//            strBuilder.append(minorVersion)
//            strBuilder.append("\n")

            Log.d(TAG, strBuilder.toString())
        }
    }
}
