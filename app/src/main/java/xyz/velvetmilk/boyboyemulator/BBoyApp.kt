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
}
