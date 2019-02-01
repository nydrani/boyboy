package xyz.velvetmilk.boyboyemulator

/**
 * @author Victor Zhang
 */
class BBoyPosRunner(private val inputInfo: MutableList<BBoyInputEvent>,
                    private val posUpdateListener: OnPosUpdateListener): Runnable {
    @Volatile private var running: Boolean = true

    override fun run() {
        while (running) {
            val posInfo: Array<BBoyInputEvent> = BBoyJNILib.obtainPos()

            inputInfo.clear()
            inputInfo.addAll(posInfo)

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
