package xyz.velvetmilk.boyboyemulator

import android.app.Application

/**
 * @author Victor Zhang
 */
class BBoyApp: Application() {

    override fun onCreate() {
        super.onCreate()

        // init java game engine
        BBoyServiceProvider.getInstance().initGameEngine()
        BBoyServiceProvider.getInstance().gameEngine.initGameLoop()
    }

    override fun onTerminate() {
        super.onTerminate()

        // TODO: Need the UI Thread to resume game engine (Change which threads control game loop)
        // NOTE: May have a deadlock if the game is "paused" (with a mutex lock) and "finishGameLoop" is called
        BBoyServiceProvider.getInstance().gameEngine.finishGameLoop()
    }
}
