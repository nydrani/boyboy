package xyz.velvetmilk.boyboyemulator

/**
 * @author Victor Zhang
 */
class BBoyServiceProvider {
    lateinit var gameEngine: BBoyGameEngineInterface

    companion object {
        @Volatile private var INSTANCE: BBoyServiceProvider? = null

        fun getInstance(): BBoyServiceProvider {
            INSTANCE ?: synchronized(this) {
                INSTANCE ?: BBoyServiceProvider().also { INSTANCE = it }
            }

            return INSTANCE!!
        }
    }

    fun initGameEngine() {
        gameEngine = BBoyGameEngineInterface()
    }
}
