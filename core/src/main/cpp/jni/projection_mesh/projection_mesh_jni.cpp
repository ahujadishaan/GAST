#include <jni.h>

#include "gdn/gast_node.h"
#include "gdn/projection_mesh/projection_mesh.h"
#include "utils.h"

// Current class and package names assumed for the Java side.
#undef JNI_PACKAGE_NAME
#define JNI_PACKAGE_NAME org_godotengine_plugin_gast_projectionmesh

#undef JNI_CLASS_NAME
#define JNI_CLASS_NAME ProjectionMesh

namespace {
using namespace gast;
using namespace godot;

inline ProjectionMesh *from_pointer(jlong projection_mesh_pointer) {
    return reinterpret_cast<ProjectionMesh *>(projection_mesh_pointer);
}

inline jlong to_pointer(ProjectionMesh *projection_mesh) {
    return reinterpret_cast<intptr_t>(projection_mesh);
}

}  // namespace

extern "C" {

JNIEXPORT jboolean JNICALL JNI_METHOD(isGazeTracking)(JNIEnv *, jobject, jlong mesh_pointer) {
    ProjectionMesh *mesh = from_pointer(mesh_pointer);
    return mesh->is_gaze_tracking();
}

JNIEXPORT void JNICALL
JNI_METHOD(setGazeTracking)(JNIEnv *, jobject, jlong mesh_pointer, jboolean gaze_tracking) {
    ProjectionMesh *mesh = from_pointer(mesh_pointer);
    mesh->set_gaze_tracking(gaze_tracking);
}

JNIEXPORT jboolean JNICALL JNI_METHOD(isRenderOnTop)(JNIEnv *, jobject, jlong mesh_pointer) {
    ProjectionMesh *mesh = from_pointer(mesh_pointer);
    return mesh->is_render_on_top();
}

JNIEXPORT void JNICALL
JNI_METHOD(setRenderOnTop)(JNIEnv *, jobject, jlong mesh_pointer, jboolean render_on_top) {
    ProjectionMesh *mesh = from_pointer(mesh_pointer);
    mesh->set_render_on_top(render_on_top);
}

JNIEXPORT jfloat JNICALL JNI_METHOD(getGastNodeGradientHeightRatio)(JNIEnv *, jobject, jlong mesh_pointer) {
    ProjectionMesh *mesh = from_pointer(mesh_pointer);
    return mesh->get_gradient_height_ratio();
}

JNIEXPORT void JNICALL
JNI_METHOD(setGastNodeGradientHeightRatio)(JNIEnv *, jobject, jlong mesh_pointer, jfloat ratio) {
    ProjectionMesh *mesh = from_pointer(mesh_pointer);
    mesh->set_gradient_height_ratio(ratio);
}

JNIEXPORT void JNICALL
JNI_METHOD(updateAlpha)(JNIEnv *, jobject, jlong mesh_pointer, jfloat alpha) {
    ProjectionMesh *mesh = from_pointer(mesh_pointer);
    mesh->set_alpha(alpha);
}

JNIEXPORT jboolean JNICALL
JNI_METHOD(nativeIsRectangular)(JNIEnv *, jobject, jlong mesh_pointer) {
    ProjectionMesh *mesh = from_pointer(mesh_pointer);
    return mesh->is_rectangular_projection_mesh();
}

JNIEXPORT jboolean JNICALL
JNI_METHOD(nativeIsEquirectangular)(JNIEnv *, jobject, jlong mesh_pointer) {
    ProjectionMesh *mesh = from_pointer(mesh_pointer);
    return mesh->is_equirectangular_projection_mesh();
}

JNIEXPORT void JNICALL
JNI_METHOD(updateGastNodeFromProjectionMesh)(JNIEnv *, jobject, jlong node_pointer) {
    GastNode *gast_node = reinterpret_cast<GastNode *>(node_pointer);
    ERR_FAIL_NULL(gast_node);
    gast_node->setup_projection_mesh();
}

}
