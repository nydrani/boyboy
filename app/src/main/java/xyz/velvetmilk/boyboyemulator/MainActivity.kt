package xyz.velvetmilk.boyboyemulator

import android.app.Activity
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.View
import android.widget.TextView
import kotlinx.android.synthetic.main.activity_main.*


class MainActivity : Activity() {
    companion object {
        private val TAG = MainActivity::class.java.simpleName
        private const val FPS_UPDATE = 1
        private const val POS_UPDATE = 2
    }

    private lateinit var surfaceView: BBoyGLSurfaceView
    private lateinit var gameEngine: BBoyGameEngineInterface

    private lateinit var openGLDebugView: TextView
    private lateinit var fpsView: TextView
    private lateinit var upsView: TextView
    private lateinit var trueupsView: TextView
    private lateinit var curFrameView: TextView
    private lateinit var curSteppedFrameView: TextView
    private lateinit var posView: TextView
    private lateinit var normPosView: TextView


    private val fpsInfo: BBoyFPS = BBoyFPS()
    private val posInfo: BBoyInputEvent = BBoyInputEvent()

    private val handler = Handler(Looper.getMainLooper()) {
        when (it.what) {
            FPS_UPDATE -> {
                updateFPSText(fpsInfo)
                true
            }
            POS_UPDATE -> {
                updatePosText(posInfo)
                true
            }
            else -> false
        }
    }


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        Log.d(TAG, "onCreate")

        onWindowFocusChanged(true)
        setContentView(R.layout.activity_main)

        surfaceView = surface_view
        openGLDebugView = opengl_version_text
        fpsView = fps_text
        upsView = ups_text
        trueupsView = true_ups_text
        curFrameView = cur_frame_text
        curSteppedFrameView = cur_stepped_frame_text
        posView = pos_text
        normPosView = norm_pos_text

        surfaceView.addOpenGLReadyListener(object : BBoyGLSurfaceView.OpenGLReadyListener {
            override fun onOpenGLReady() {
                runOnUiThread {
                    setOpenGLDebugInfo()
                }
            }
        })


        gameEngine = BBoyServiceProvider.getInstance().gameEngine
        gameEngine.runGameLoop()

        pause_button.setOnClickListener { gameEngine.pauseGameLoop() }
        resume_button.setOnClickListener { gameEngine.resumeGameLoop() }
        pause_engine_button.setOnClickListener { gameEngine.pauseGameEngine() }
        resume_engine_button.setOnClickListener { gameEngine.resumeGameEngine() }
    }

    override fun onStart() {
        super.onStart()
        Log.d(TAG, "onStart")

        surfaceView.onResume()
        gameEngine.initFPSRunner(fpsInfo, object : BBoyFPSRunner.OnFPSUpdateListener {
            override fun onFPSUpdate() {
                handler.sendEmptyMessage(FPS_UPDATE)
            }
        })
        gameEngine.initPosRunner(posInfo, object : BBoyPosRunner.OnPosUpdateListener {
            override fun onPosUpdate() {
                handler.sendEmptyMessage(POS_UPDATE)
            }
        })

        gameEngine.resumeGameEngine()
    }

    override fun onStop() {
        super.onStop()
        Log.d(TAG, "onStop")

        surfaceView.onPause()
        gameEngine.shutdownFPSRunner()
        gameEngine.shutdownPosRunner()

        gameEngine.pauseGameEngine()
    }

    override fun onDestroy() {
        super.onDestroy()
        Log.d(TAG, "onDestroy")

        // any occasion where onDestroy does not get called is when the process will be nuked
        // from existence so the thread leaking doesn't seem possible
        gameEngine.finishGameLoop()
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)

        if (hasFocus) {
            hideSystemUI()
        }
    }

    private fun hideSystemUI() {
        // Enables regular immersive mode.
        // For "lean back" mode, remove SYSTEM_UI_FLAG_IMMERSIVE.
        // Or for "sticky immersive," replace it with SYSTEM_UI_FLAG_IMMERSIVE_STICKY
        window.decorView.systemUiVisibility = (View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                // Set the content to appear under the system bars so that the
                // content doesn't resize when the system bars hide and show.
                or View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                // Hide the nav bar and status bar
                or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_FULLSCREEN)
    }

    // Shows the system bars by removing all the flags
    // except for the ones that make the content appear under the system bars.
    private fun showSystemUI() {
        window.decorView.systemUiVisibility = (View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN)
    }

    private fun setOpenGLDebugInfo() {
        val version = surfaceView.openGLVersion
        val renderer = surfaceView.openGLRenderer

        openGLDebugView.text = version + "\n" + renderer
    }

    private fun updateFPSText(fpsInfo: BBoyFPS) {
        fpsView.text = "FPS: " + fpsInfo.fps.toString()
        upsView.text = "UPS: " + fpsInfo.ups.toString()
        trueupsView.text = "TRUE_UPS: " + fpsInfo.true_ups.toString()
        curFrameView.text = "Frame #: " + fpsInfo.frame.toString()
        curSteppedFrameView.text = "Stepped Frame #: " + fpsInfo.stepped_frame.toString()
    }

    private fun updatePosText(posInfo: BBoyInputEvent) {
        posView.text = "x: " + posInfo.x.toString() + " | y: " + posInfo.y.toString()
        normPosView.text = "norm x: " + posInfo.normX.toString() + " | norm y: " + posInfo.normY.toString()
    }
}
