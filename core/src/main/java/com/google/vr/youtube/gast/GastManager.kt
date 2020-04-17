@file:JvmName("GastManager")

package com.google.vr.youtube.gast

import android.app.Activity
import android.util.Log
import android.widget.FrameLayout
import org.godotengine.godot.Godot
import org.godotengine.godot.plugin.GodotPlugin
import java.util.concurrent.ConcurrentLinkedQueue
import javax.microedition.khronos.opengles.GL10

/**
 * GAST core plugin.
 *
 * Provides the functionality for rendering, interacting and manipulating content generated by the
 * Android system onto Godot textures.
 */
class GastManager(godot: Godot) : GodotPlugin(godot) {

    /**
     * Used to listen to events dispatched by the Gast plugin.
     */
    interface GastEventListener {
        /**
         * Forward the '_process' callback of the Godot node with the given 'nodePath'
         * Invoked on the render thread.
         */
        fun onRenderProcess(nodePath: String, delta: Float)

        /**
         * Forward the render draw frame callback.
         */
        fun onRenderDrawFrame()
    }

    init {
        System.loadLibrary("gast")
    }

    private val gastEventListeners = ConcurrentLinkedQueue<GastEventListener>()

    /**
     * Root parent for all GAST views.
     */
    val rootView = FrameLayout(godot)

    companion object {
        private val TAG = GastManager::class.java.simpleName
        private const val INVALID_SURFACE_INDEX = -1
    }

    override fun onGodotMainLoopStarted() {
        Log.d(TAG, "Initializing ${pluginName} manager")
        initialize()
    }

    override fun onMainCreateView(activity: Activity) = rootView

    override fun onMainDestroy() {
        Log.d(TAG, "Shutting down ${pluginName} manager")
        runOnRenderThread { shutdown() }
    }

    override fun onGLDrawFrame(gl: GL10) {
        for (listener in gastEventListeners) {
            listener.onRenderDrawFrame()
        }
    }

    override fun getPluginMethods(): MutableList<String> = emptyList<String>().toMutableList()

    override fun getPluginName() = "gast-core"

    override fun getPluginGDNativeLibrariesPaths() = setOf("godot/plugin/v1/gast/gastlib.gdnlib")

    fun addGastEventListener(listener: GastEventListener) {
        gastEventListeners += listener;
    }

    fun removeGastEventListener(listener: GastEventListener) {
        gastEventListeners -= listener
    }

    @JvmOverloads
    external fun getExternalTextureId(nodePath: String, surfaceIndex: Int = INVALID_SURFACE_INDEX): Int

    /**
     * Create a mesh instance with the given parent node and set it up.
     * @param parentNodePath - Path to the parent for the mesh instance that will be created. The parent node must exist
     * @return The node path to the newly created mesh instance
     */
    external fun acquireAndBindQuadMeshInstance(parentNodePath: String): String

    /**
     * Setup the mesh instance with the given node path for GAST view support.
     * @param nodePath - Path to an existing mesh instance node
     */
    external fun bindMeshInstance(nodePath: String): Boolean

    /**
     * Unbind the mesh instance with the given node path. This is the counterpart to [GastManager.bindMeshInstance]
     */
    external fun unbindMeshInstance(nodePath: String)

    /**
     * Unbind and release the mesh instance with the given node path. This is the counterpart to [GastManager.acquireAndBindQuadMeshInstance]
     */
    external fun unbindAndReleaseQuadMeshInstance(nodePath: String)

    external fun updateNodeParent(nodePath: String, newParentNodePath: String): String

    external fun updateSpatialNodeVisibility(
        nodePath: String,
        shouldDuplicateParentVisibility: Boolean,
        visible: Boolean
    )

    external fun updateQuadMeshInstanceSize(nodePath: String, width: Float, height: Float)

    external fun updateSpatialNodeLocalTranslation(
        nodePath: String,
        xTranslation: Float,
        yTranslation: Float,
        zTranslation: Float
    )

    external fun updateSpatialNodeLocalScale(nodePath: String, xScale: Float, yScale: Float)

    external fun updateSpatialNodeLocalRotation(
        nodePath: String,
        xRotation: Float,
        yRotation: Float,
        zRotation: Float
    )

    private external fun initialize()

    private external fun shutdown()

    /**
     * Invoked by the native layer to forward the node's '_process' callback.
     */
    private fun onRenderProcess(nodePath: String, delta: Float) {
        for (listener in gastEventListeners) {
            listener.onRenderProcess(nodePath, delta)
        }
    }

    private fun onRenderInputHover(nodePath: String, xPercent: Float, yPercent: Float) {}

    private fun onRenderInputPress(nodePath: String, xPercent: Float, yPercent: Float) {}

    private fun onRenderInputRelease(nodePath: String, xPercent: Float, yPercent: Float) {}

}