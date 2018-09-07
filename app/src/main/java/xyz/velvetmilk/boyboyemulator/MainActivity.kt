package xyz.velvetmilk.boyboyemulator

import android.app.Activity
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.os.Message
import android.util.Log
import android.view.View
import android.widget.Button
import android.widget.TextView


class MainActivity : Activity(), BBoyFPSRunner.OnFPSUpdateListener, BBoyPosRunner.OnPosUpdateListener {
    companion object {
        private val TAG = MainActivity::class.java.simpleName
        private const val FPS_UPDATE = 1
        private const val POS_UPDATE = 2
    }

    private lateinit var surfaceView: BBoyGLSurfaceView
    private lateinit var gameEngine: BBoyGameEngineInterface

    private lateinit var fpsView: TextView
    private lateinit var upsView: TextView
    private lateinit var trueupsView: TextView
    private lateinit var posView: TextView


    private val fpsInfo = BBoyFPS()
    private val posInfo = BBoyInputEvent()

    private val handler = object : Handler(Looper.getMainLooper()) {
        override fun handleMessage(msg: Message) {
            when (msg.what) {
                FPS_UPDATE -> {
                    updateFPSText(fpsInfo)
                }
                POS_UPDATE -> {
                    updatePosText(posInfo)
                }
            }
        }
    }


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        Log.d(TAG, "onCreate")

        onWindowFocusChanged(true)
        setContentView(R.layout.activity_main)

        surfaceView = findViewById(R.id.surface_view)
        fpsView = findViewById(R.id.fps_text)
        upsView = findViewById(R.id.ups_text)
        trueupsView = findViewById(R.id.true_ups_text)
        posView = findViewById(R.id.pos_text)

        gameEngine = BBoyServiceProvider.getInstance().gameEngine
        gameEngine.runGameLoop()

        val pause_button: Button = findViewById(R.id.pause_button)
        pause_button.setOnClickListener { gameEngine.pauseGameLoop() }

        val resume_button: Button = findViewById(R.id.resume_button)
        resume_button.setOnClickListener { gameEngine.resumeGameLoop() }
    }

    override fun onStart() {
        super.onStart()
        Log.d(TAG, "onStart")

        surfaceView.onResume()
        gameEngine.initFPSRunner(fpsInfo, this)
        gameEngine.initPosRunner(posInfo, this)
    }

    override fun onStop() {
        super.onStop()
        Log.d(TAG, "onStop")

        surfaceView.onPause()
        gameEngine.shutdownFPSRunner()
        gameEngine.shutdownPosRunner()
    }

    override fun onDestroy() {
        super.onDestroy()
        Log.d(TAG, "onDestroy")

        // any occasion where onDestroy does not get called is when the process will be nuked from existance
        // so the thread leaking doesn't seem possible
        gameEngine.finishGameLoop()
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)

        if (hasFocus) {
            hideSystemUI()
        }
    }

    override fun onFPSUpdate() {
        handler.sendEmptyMessage(FPS_UPDATE)
    }

    override fun onPosUpdate() {
        handler.sendEmptyMessage(POS_UPDATE)
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

    fun updateFPSText(fpsInfo: BBoyFPS?) {
        fpsInfo?.let {
            fpsView.text = "FPS: " + it.fps.toString()
            upsView.text = "UPS: " + it.ups.toString()
            trueupsView.text = "TRUE_UPS: " + it.true_ups.toString()
        }
    }

    fun updatePosText(posInfo: BBoyInputEvent?) {
        posInfo?.let {
            posView.text = "x: " + it.x.toString() + " | y: " + it.y.toString()
        }
    }
}
