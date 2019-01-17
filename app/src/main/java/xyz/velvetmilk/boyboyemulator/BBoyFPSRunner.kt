package xyz.velvetmilk.boyboyemulator

/**
 * @author Victor Zhang
 */
class BBoyFPSRunner(private val fpsInfo: BBoyFPS,
                    private val fpsUpdateListener: OnFPSUpdateListener): Runnable {
    @Volatile private var running: Boolean = true

    override fun run() {
        while (running) {
            BBoyJNILib.obtainFPS(fpsInfo)

            fpsUpdateListener.onFPSUpdate()

            try {
                Thread.sleep(20)
            } catch (e: InterruptedException) {
                e.printStackTrace()
            }
        }
    }

    fun shutdown() {
        running = false
    }

    interface OnFPSUpdateListener {
        fun onFPSUpdate()
    }
}
