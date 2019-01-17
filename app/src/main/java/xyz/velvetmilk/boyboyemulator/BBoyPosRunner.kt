package xyz.velvetmilk.boyboyemulator

/**
 * @author Victor Zhang
 */
class BBoyPosRunner(private val inputInfo: BBoyInputEvent,
                    private val posUpdateListener: OnPosUpdateListener): Runnable {
    @Volatile private var running: Boolean = true

    override fun run() {
        while (running) {
            BBoyJNILib.obtainPos(inputInfo)

            posUpdateListener.onPosUpdate()

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

    interface OnPosUpdateListener {
        fun onPosUpdate()
    }
}
