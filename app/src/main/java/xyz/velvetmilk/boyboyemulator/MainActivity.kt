package xyz.velvetmilk.boyboyemulator

import android.app.Activity
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.View
import kotlinx.android.synthetic.main.activity_main.*


class MainActivity : Activity() {
    companion object {
        private val TAG = MainActivity::class.java.simpleName
        private const val FPS_UPDATE = 1
        private const val POS_UPDATE = 2
        private const val DEBUGGING = false
    }

    private lateinit var gameEngine: BBoyGameEngineInterface

    private val fpsInfo: BBoyFPS = BBoyFPS()
    private val posInfo: MutableList<BBoyInputEvent> = mutableListOf()

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

        if (!DEBUGGING) {
            opengl_version_text.visibility = View.GONE
            fps_text.visibility = View.GONE
            sps_text.visibility = View.GONE
            ups_text.visibility = View.GONE
            true_ups_text.visibility = View.GONE
            cur_frame_text.visibility = View.GONE
            cur_stepped_frame_text.visibility = View.GONE
            cur_time_text.visibility = View.GONE
            pos_text.visibility = View.GONE
            norm_pos_text.visibility = View.GONE
            pause_button.visibility = View.GONE
            resume_button.visibility = View.GONE
            resume_engine_button.visibility = View.GONE
            pause_engine_button.visibility = View.GONE
        }

        surface_view.addOpenGLReadyListener(object : BBoyGLSurfaceView.OpenGLReadyListener {
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

        surface_view.onResume()
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

        surface_view.onPause()
        gameEngine.shutdownFPSRunner()
        gameEngine.shutdownPosRunner()

        gameEngine.pauseGameEngine()
    }

    override fun onDestroy() {
        super.onDestroy()
        Log.d(TAG, "onDestroy")

        // on rotation (this gets called)
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
        val version = surface_view.openGLVersion
        val renderer = surface_view.openGLRenderer

        opengl_version_text.text = version + "\n" + renderer
    }

    private fun updateFPSText(fpsInfo: BBoyFPS) {
        fps_text.text = "FPS: " + fpsInfo.fps.toString()
        sps_text.text = "SPS: " + fpsInfo.sps.toString()
        ups_text.text = "UPS: " + fpsInfo.ups.toString()
        true_ups_text.text = "TRUE_UPS: " + fpsInfo.true_ups.toString()
        cur_frame_text.text = "Frame #: " + fpsInfo.frame.toString()
        cur_stepped_frame_text.text = "Stepped Frame #: " + fpsInfo.stepped_frame.toString()
        cur_time_text.text = "Time: " + fpsInfo.cur_time.toString()
    }

    private fun updatePosText(posInfo: List<BBoyInputEvent>) {
        if (posInfo.isEmpty()) {
            return
        }

        val stringBuilder = StringBuilder()
        val normStringBuilder = StringBuilder()
        for ((index, event) in posInfo.withIndex()) {
            stringBuilder.appendln(index.toString() + " | x: " + event.x.toString() + " | y: " + event.y.toString())
            normStringBuilder.appendln(index.toString() + " | norm x: " + event.normX.toString() + " | norm y: " + event.normY.toString())
        }

        stringBuilder.deleteCharAt(stringBuilder.length-1)
        normStringBuilder.deleteCharAt(normStringBuilder.length-1)

        pos_text.text = stringBuilder.toString()
        norm_pos_text.text = normStringBuilder.toString()
    }
}
