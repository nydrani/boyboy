package xyz.velvetmilk.boyboyemulator

import android.os.Parcelable
import kotlinx.android.parcel.Parcelize

/**
 * @author Victor Zhang
 */
@Parcelize
data class BBoyFPS(var fps: Double = 0.0, var ups: Double = 0.0, var true_ups: Double = 0.0) : Parcelable {

    override fun toString(): String {
        return fps.toString() + " " + ups.toString() + " " + true_ups.toString()
    }
}