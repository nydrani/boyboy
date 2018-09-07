package xyz.velvetmilk.boyboyemulator

import android.os.Parcelable
import kotlinx.android.parcel.Parcelize

/**
 * @author Victor Zhang
 */
@Parcelize
data class BBoyInputEvent(var x: Float = 0.0f, var y: Float = 0.0f) : Parcelable {

    override fun toString(): String {
        return x.toString() + " " + y.toString()
    }
}