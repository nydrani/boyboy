package xyz.velvetmilk.boyboyemulator

import android.os.Parcelable
import kotlinx.android.parcel.Parcelize

/**
 * @author Victor Zhang
 */
@Parcelize
data class BBoyFPS(var fps: Float = 0.0f, var ups: Float = 0.0f, var true_ups: Float = 0.0f, var frame: Long = 0, var stepped_frame: Long = 0, var cur_time: Long = 0) : Parcelable {

    override fun toString(): String {
        return fps.toString() + " " + ups.toString() + " " + true_ups.toString() + " " + frame.toString() + " " + stepped_frame.toString() + " " + cur_time.toString()
    }
}
