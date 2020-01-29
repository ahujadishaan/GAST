#include <gen/Engine.hpp>
#include <gen/MainLoop.hpp>
#include <gen/Material.hpp>
#include <gen/SceneTree.hpp>
#include <gen/ShaderMaterial.hpp>
#include <gen/Object.hpp>
#include <core/Godot.hpp>
#include <gen/Texture.hpp>
#include "gast_node_manager.h"
#include "utils.h"

namespace gast {

    namespace {
        constexpr int kInvalidTexId = -1;
    } // namespace

    GastNodeManager *GastNodeManager::singleton_instance = nullptr;

    GastNodeManager *GastNodeManager::get_singleton_instance() {
        if (singleton_instance == nullptr) {
            singleton_instance = new GastNodeManager();
        }
        return singleton_instance;
    }

    void GastNodeManager::delete_singleton_instance() {
        delete singleton_instance;
        singleton_instance = nullptr;
    }

    GastNodeManager::GastNodeManager() {}

    GastNodeManager::~GastNodeManager() {}

    void GastNodeManager::register_callback(JNIEnv *env, jobject callback) {
        callback_instance = env->NewGlobalRef(callback);
        ALOG_ASSERT(callback_instance != nullptr, "Invalid value for callback.");

        jclass callback_class = env->GetObjectClass(callback_instance);
        ALOG_ASSERT(callback_class != nullptr, "Invalid value for callback.");

        on_gl_process_ = env->GetMethodID(callback_class, "onGLProcess", "(F)V");
        ALOG_ASSERT(on_gl_process != nullptr, "Unable to find onGLProcess");

        on_gl_input_hover_ = env->GetMethodID(callback_class, "onGLInputHover", "(FF)V");
        ALOG_ASSERT(on_gl_input_hover_ != nullptr, "Unable to find onGLInputHover");

        on_gl_input_press_ = env->GetMethodID(callback_class, "onGLInputPress", "(FF)V");
        ALOG_ASSERT(on_gl_input_press_ != nullptr, "Unable to find onGLInputPress");

        on_gl_input_release_ = env->GetMethodID(callback_class, "onGLInputRelease", "(FF)V");
        ALOG_ASSERT(on_gl_input_release_ != nullptr, "Unable to find onGLInputRelease");
    }

    void GastNodeManager::unregister_callback(JNIEnv *env) {
        if (callback_instance) {
            env->DeleteGlobalRef(callback_instance);
            callback_instance = nullptr;
        }
    }

    int GastNodeManager::get_external_texture_id(const String &mesh_name,
                                             const String &texture_param_name) {
        ExternalTexture *external_texture = get_external_texture(mesh_name, texture_param_name);
        int tex_id = external_texture == nullptr ? kInvalidTexId
                                                 : external_texture->get_external_texture_id();
        return tex_id;
    }

    ExternalTexture *GastNodeManager::get_external_texture(const String &mesh_name,
                                                       const String &texture_param_name) {
        // Go through the mesh instance surface material and look for the external texture.
        ALOGV("Looking for external texture %s", texture_param_name.utf8().get_data());
        MeshInstance *mesh_instance = get_mesh_instance(mesh_name);
        if (!mesh_instance) {
            return nullptr;
        }

        for (int i = 0; i < mesh_instance->get_surface_material_count(); i++) {
            Ref<Material> material = mesh_instance->get_surface_material(i);
            if (material.is_null() || !material->is_class("ShaderMaterial")) {
                continue;
            }

            auto *shader_material = Object::cast_to<ShaderMaterial>(*material);
            Ref<Texture> texture = shader_material->get_shader_param(texture_param_name);
            if (texture.is_null() || !texture->is_class("ExternalTexture")) {
                continue;
            }

            auto *external_texture = Object::cast_to<ExternalTexture>(*texture);
            ALOGV("Found external texture %s", texture_param_name.utf8().get_data());
            return external_texture;
        }
        return nullptr;
    }

    MeshInstance *GastNodeManager::get_mesh_instance(const godot::String &mesh_name) {
        MainLoop *main_loop = Engine::get_singleton()->get_main_loop();
        if (!main_loop || !main_loop->is_class("SceneTree")) {
            ALOGE("Unable to retrieve main loop.");
            return nullptr;
        }

        auto *scene_tree = Object::cast_to<SceneTree>(main_loop);

        // Retrieve the nodes that are part of the given group.
        ALOGV("Retrieving nodes for %s", mesh_name.utf8().get_data());
        Array nodes = scene_tree->get_nodes_in_group(mesh_name);
        if (nodes.empty()) {
            ALOGW("No nodes in group %s", mesh_name.utf8().get_data());
            return nullptr;
        }

        // Return the first node that is a mesh instance.
        for (int i = 0; i < nodes.size(); i++) {
            Node *node = nodes[i];

            // Check if the node is a mesh instance.
            if (!node || !node->is_class("MeshInstance")) {
                continue;
            }

            auto *mesh_instance = Object::cast_to<MeshInstance>(node);
            return mesh_instance;
        }
        return nullptr;
    }

    void GastNodeManager::on_gl_process(float delta) {
        if (callback_instance && on_gl_process_) {
            JNIEnv *env = godot::android_api->godot_android_get_env();
            env->CallVoidMethod(callback_instance, on_gl_process_, delta);
        }
    }

    void GastNodeManager::on_gl_input_hover(float x_percent, float y_percent) {
        if (callback_instance && on_gl_input_hover_) {
            JNIEnv *env = godot::android_api->godot_android_get_env();
            env->CallVoidMethod(callback_instance, on_gl_input_hover_, x_percent, y_percent);
        }
    }

    void GastNodeManager::on_gl_input_press(float x_percent, float y_percent) {
        if (callback_instance && on_gl_input_press_) {
            JNIEnv *env = godot::android_api->godot_android_get_env();
            env->CallVoidMethod(callback_instance, on_gl_input_press_, x_percent, y_percent);
        }
    }

    void GastNodeManager::on_gl_input_release(float x_percent, float y_percent) {
        if (callback_instance && on_gl_input_release_) {
            JNIEnv *env = godot::android_api->godot_android_get_env();
            env->CallVoidMethod(callback_instance, on_gl_input_release_, x_percent, y_percent);
        }
    }

}  // namespace gast