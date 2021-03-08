package org.godotengine.plugin.gast.samples.video

import org.godotengine.godot.Godot
import org.godotengine.godot.xr.XRMode
import java.util.*

class MainActivity : Godot() {

    // Override the engine starting parameters to indicate we want VR mode.
    override fun getCommandLine(): Array<String> {
        val original = super.getCommandLine()
        if (BuildConfig.FLAVOR == "ovr") {
            val updated: Array<String> = Arrays.copyOf(original, original.size + 1)
            updated[original.size] = XRMode.OVR.cmdLineArg
            return updated
        } else {
            return original
        }
    }
}
