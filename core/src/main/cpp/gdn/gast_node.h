#ifndef GAST_NODE_H
#define GAST_NODE_H

#include <core/Godot.hpp>
#include <core/Ref.hpp>
#include <core/Vector2.hpp>
#include <core/Vector3.hpp>
#include <gen/CollisionShape.hpp>
#include <gen/ConcavePolygonShape.hpp>
#include <gen/ExternalTexture.hpp>
#include <gen/InputEvent.hpp>
#include <gen/MeshInstance.hpp>
#include <gen/Mesh.hpp>
#include <gen/Object.hpp>
#include <gen/PlaneMesh.hpp>
#include <gen/QuadMesh.hpp>
#include <gen/RayCast.hpp>
#include <gen/Shader.hpp>
#include <gen/ShaderMaterial.hpp>
#include <gen/StaticBody.hpp>

#include "gdn/projection_mesh/projection_mesh.h"
#include "gdn/projection_mesh/projection_mesh_pool.h"
#include "utils.h"

namespace gast {

namespace {
using namespace godot;
constexpr int kInvalidTexId = -1;
constexpr int kInvalidSurfaceIndex = -1;
const bool kDefaultCollidable = true;
const bool kDefaultGazeTracking = false;
const bool kDefaultRenderOnTop = false;
const float kDefaultGradientHeightRatio = 0.0f;
}  // namespace

/// Script for a GAST node. Enables GAST specific logic and processing.
class GastNode : public StaticBody {
GODOT_CLASS(GastNode, StaticBody)

public:
    GastNode();

    ~GastNode();

    static void _register_methods();

    void _init();

    void _enter_tree();

    void _exit_tree();

    void
    _input_event(const Object *camera, const Ref<InputEvent> event, const Vector3 click_position,
                 const Vector3 click_normal, const int64_t shape_idx);

    void _process(const real_t delta);

    void _notification(const int64_t what);

    int get_external_texture_id(int surface_index = kInvalidSurfaceIndex);

    inline void set_collidable(bool collidable) {
        if (this->collidable == collidable) {
            return;
        }
        this->collidable = collidable;
        update_collision_shape();
    }

    inline bool is_collidable() {
        return collidable;
    }

    inline void set_projection_mesh(int projection_mesh_type) {
        set_projection_mesh(static_cast<ProjectionMesh::ProjectionMeshType>(projection_mesh_type));
    }

    inline void set_projection_mesh(ProjectionMesh::ProjectionMeshType projection_mesh_type) {
        if (projection_mesh_type == get_projection_mesh()->get_projection_mesh_type()) {
            return;
        }

        if (projection_mesh_type == ProjectionMesh::ProjectionMeshType::RECTANGULAR) {
            projection_mesh = projection_mesh_pool.get_or_create_rectangular_projection_mesh();
        } else if (projection_mesh_type == ProjectionMesh::ProjectionMeshType::EQUIRECTANGULAR) {
            projection_mesh = projection_mesh_pool.get_or_create_equirectangular_projection_mesh();
        }
        setup_projection_mesh();
    }

    inline ProjectionMesh* get_projection_mesh() {
        return this->projection_mesh;
    }

    inline void set_render_on_top(bool enable) {
        projection_mesh->set_render_on_top(enable);
    }

    inline bool is_render_on_top() {
        return projection_mesh->is_render_on_top();
    }

    inline void set_gaze_tracking(bool gaze_tracking) {
        projection_mesh->set_gaze_tracking(gaze_tracking);
    }

    inline bool is_gaze_tracking() {
        return projection_mesh->is_gaze_tracking();
    }

    inline float get_gradient_height_ratio() {
        return projection_mesh->get_gradient_height_ratio();
    }

    inline void set_gradient_height_ratio(float ratio) {
        projection_mesh->set_gradient_height_ratio(ratio);
    }

    inline void set_alpha(float alpha) {
        projection_mesh->set_alpha(alpha);
    }

    static inline RayCast *get_ray_cast_from_variant(Variant variant) {
        RayCast *ray_cast = Object::cast_to<RayCast>(variant);
        return ray_cast;
    }

    Vector2 get_relative_collision_point(Vector3 absolute_collision_point);

    // Handle the raycast input. Returns true if a press is in progress.
    bool handle_ray_cast_input(const String &ray_cast_name, Vector2 relative_collision_point);

    // Returns true if the plane defined by this node intersects the given ray.
    bool intersects_ray(Vector3 ray_origin, Vector3 ray_direction, Vector3 *intersection);

    void setup_projection_mesh();

private:

    inline CollisionShape *get_collision_shape() {
        Node *node = get_child(0);
        CollisionShape *collision_shape = Object::cast_to<CollisionShape>(node);
        return collision_shape;
    }

    static inline String get_click_action_from_node_name(const String &node_name) {
        // Replace the '/' character with a '_' character
        return node_name.replace("/", "_") + "_click";
    }

    static inline String get_horizontal_left_scroll_action_from_node_name(const String &node_name) {
        // Replace the '/' character with a '_' character
        return node_name.replace("/", "_") + "_left_scroll";
    }

    static inline String
    get_horizontal_right_scroll_action_from_node_name(const String &node_name) {
        // Replace the '/' character with a '_' character
        return node_name.replace("/", "_") + "_right_scroll";
    }

    static inline String get_vertical_up_scroll_action_from_node_name(const String &node_name) {
        // Replace the '/' character with a '_' character
        return node_name.replace("/", "_") + "_up_scroll";
    }

    static inline String get_vertical_down_scroll_action_from_node_name(const String &node_name) {
        // Replace the '/' character with a '_' character
        return node_name.replace("/", "_") + "_down_scroll";
    }

    ExternalTexture *get_external_texture(int surface_index);

    void update_collision_shape();

    void reset_mesh_and_collision_shape();

    bool collidable;
    ProjectionMeshPool projection_mesh_pool;
    ProjectionMesh *projection_mesh;
    ExternalTexture *external_texture;
};

}  // namespace gast

#endif // GAST_NODE_H
