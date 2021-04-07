#include "gast_node.h"
#include "gast_manager.h"
#include "gdn/projection_mesh/rectangular_projection_mesh.h"
#include <utils.h>

#include <core/Array.hpp>
#include <core/String.hpp>
#include <core/NodePath.hpp>
#include <core/Rect2.hpp>
#include <core/Transform.hpp>
#include <gen/Camera.hpp>
#include <gen/Input.hpp>
#include <gen/InputEventScreenDrag.hpp>
#include <gen/InputEventScreenTouch.hpp>
#include <gen/Material.hpp>
#include <gen/Mesh.hpp>
#include <gen/Node.hpp>
#include <gen/QuadMesh.hpp>
#include <gen/ArrayMesh.hpp>
#include <gen/Resource.hpp>
#include <gen/ResourceLoader.hpp>
#include <gen/SceneTree.hpp>
#include <gen/Shape.hpp>
#include <gen/Texture.hpp>
#include <gen/Viewport.hpp>

namespace gast {

namespace {
const char *kCapturedGastRayCastGroupName = "captured_gast_ray_casts";
}

GastNode::GastNode() : collidable(kDefaultCollidable),
                       projection_mesh_pool(ProjectionMeshPool()) {
    projection_mesh = projection_mesh_pool.get_or_create_rectangular_projection_mesh();
}

GastNode::~GastNode() = default;

void GastNode::_register_methods() {
    register_method("_enter_tree", &GastNode::_enter_tree);
    register_method("_exit_tree", &GastNode::_exit_tree);
    register_method("_input_event", &GastNode::_input_event);
    register_method("_physics_process", &GastNode::_physics_process);
    register_method("_process", &GastNode::_process);
    register_method("_notification", &GastNode::_notification);

    register_method("set_collidable", &GastNode::set_collidable);
    register_method("is_collidable", &GastNode::is_collidable);
    register_method("set_gaze_tracking", &GastNode::set_gaze_tracking);
    register_method("is_gaze_tracking", &GastNode::is_gaze_tracking);
    register_method("set_render_on_top", &GastNode::set_render_on_top);
    register_method("is_render_on_top", &GastNode::is_render_on_top);
    register_method("set_gradient_height_ratio", &GastNode::set_gradient_height_ratio);
    register_method("get_gradient_height_ratio", &GastNode::get_gradient_height_ratio);
    register_method("get_external_texture_id", &GastNode::get_external_texture_id);

    register_property<GastNode, bool>("collidable", &GastNode::set_collidable,
                                      &GastNode::is_collidable, kDefaultCollidable);
    register_property<GastNode, bool>("gaze_tracking", &GastNode::set_gaze_tracking,
                                      &GastNode::is_gaze_tracking, kDefaultGazeTracking);
    register_property<GastNode, bool>("render_on_top", &GastNode::set_render_on_top,
                                      &GastNode::is_render_on_top, kDefaultRenderOnTop);
    register_property<GastNode, float>("gradient_height_ratio",
                                       &GastNode::set_gradient_height_ratio,
                                       &GastNode::get_gradient_height_ratio,
                                       kDefaultGradientHeightRatio);
}

void GastNode::_init() {
    ALOGV("Initializing GastNode class.");

    // Add a CollisionShape to the static body node
    CollisionShape *collision_shape = CollisionShape::_new();
    add_child(collision_shape);
}

void GastNode::_enter_tree() {
    ALOGV("Entering tree for %s.", get_node_tag(*this));

    // Create the external texture
    external_texture = ExternalTexture::_new();

    setup_projection_mesh();
}

void GastNode::_exit_tree() {
    ALOGV("Exiting tree.");
    reset_mesh_and_collision_shape();
}

void GastNode::reset_mesh_and_collision_shape() {
    // Unset the GAST mesh resource
    projection_mesh->reset_mesh();
    // Unset the box shape resource
    update_collision_shape();
}

void GastNode::setup_projection_mesh() {
    CollisionShape *collisionShape = get_collision_shape();
    for (int i = 0; i < collisionShape->get_child_count(); i++) {
        collisionShape->remove_child(collisionShape->get_child(i));
    }
    collisionShape->add_child(projection_mesh->get_mesh_instance());
    projection_mesh->set_external_texture(external_texture);
    update_collision_shape();
    projection_mesh->update_render_priority();
}

void GastNode::update_collision_shape() {
    CollisionShape *collision_shape = get_collision_shape();
    if (!collision_shape) {
        ALOGW("Unable to retrieve collision shape for %s. Aborting...", get_node_tag(*this));
        return;
    }

    if (!is_visible_in_tree() || !collidable || projection_mesh->get_mesh_instance() == nullptr) {
        collision_shape->set_shape(Ref<Resource>());
    } else {
        collision_shape->set_shape(projection_mesh->get_collision_shape());
    }
}

int GastNode::get_external_texture_id(int surface_index) {
    if (surface_index == kInvalidSurfaceIndex) {
        // Default to the first one
        surface_index = kDefaultSurfaceIndex;
    }

    ExternalTexture *external_texture = get_external_texture(surface_index);
    int tex_id = external_texture == nullptr ? kInvalidTexId
                                             : external_texture->get_external_texture_id();
    ALOGV("Retrieved tex id %d", tex_id);
    return tex_id;
}

ExternalTexture *GastNode::get_external_texture(int surface_index) {
    return external_texture;
}

void GastNode::_input_event(const godot::Object *camera,
                            const godot::Ref<godot::InputEvent> event,
                            const godot::Vector3 click_position,
                            const godot::Vector3 click_normal, const int64_t shape_idx) {
    if (event.is_null()) {
        return;
    }

    String node_path = get_path();

    // Calculate the 2D collision point of the raycast on the Gast node.
    Vector2 relative_collision_point = get_relative_collision_point(click_position);
    float x_percent = relative_collision_point.x;
    float y_percent = relative_collision_point.y;

    // This should only fire for touch screen input events, so we filter for those.
    if (event->is_class(InputEventScreenTouch::___get_class_name())) {
        auto *touch_event = Object::cast_to<InputEventScreenTouch>(*event);
        if (touch_event) {
            String touch_event_id = InputEventScreenTouch::___get_class_name() +
                                    String::num_int64(touch_event->get_index());
            if (touch_event->is_pressed()) {
                GastManager::get_singleton_instance()->on_render_input_press(node_path,
                                                                             touch_event_id,
                                                                             x_percent,
                                                                             y_percent);
            } else {
                GastManager::get_singleton_instance()->on_render_input_release(node_path,
                                                                               touch_event_id,
                                                                               x_percent,
                                                                               y_percent);
            }
        }
    } else if (event->is_class(InputEventScreenDrag::___get_class_name())) {
        auto *drag_event = Object::cast_to<InputEventScreenDrag>(*event);
        if (drag_event) {
            String drag_event_id = InputEventScreenDrag::___get_class_name() +
                                   String::num_int64(drag_event->get_index());
            GastManager::get_singleton_instance()->on_render_input_hover(node_path,
                                                                         drag_event_id, x_percent,
                                                                         y_percent);
        }
    }
}

void GastNode::_notification(const int64_t what) {
    switch(what) {
        case NOTIFICATION_VISIBILITY_CHANGED:
            update_collision_shape();
            break;
    }
}

void GastNode::_process(const real_t delta) {
    if (projection_mesh->is_gaze_tracking()) {
        Rect2 gaze_area = get_viewport()->get_visible_rect();
        Vector2 gaze_center_point = Vector2(gaze_area.position.x + gaze_area.size.x / 2.0,
                                            gaze_area.position.y + gaze_area.size.y / 2.0);

        // Get the distance between the camera and this node.
        Camera* camera = get_viewport()->get_camera();
        Transform global_transform = get_global_transform();
        float distance = camera->get_global_transform().origin.distance_to(global_transform.origin);

        // Update the node's position to match the center of the gaze area.
        Vector3 updated_position = camera->project_position(gaze_center_point, distance);
        global_transform.origin = updated_position;
        set_global_transform(global_transform);
    }
}

void GastNode::_physics_process(const real_t delta) {
    if (!is_collidable()) {
        return;
    }

    // Get the list of ray casts in the group
    Array gast_ray_casts = get_tree()->get_nodes_in_group(kGastRayCasterGroupName);
    if (gast_ray_casts.empty()) {
        return;
    }

    NodePath node_path = get_path();
    for (int i = 0; i < gast_ray_casts.size(); i++) {
        RayCast *ray_cast = get_ray_cast_from_variant(gast_ray_casts[i]);
        if (!ray_cast || !ray_cast->is_enabled()) {
            continue;
        }

        String ray_cast_path = ray_cast->get_path();

        // Check if the raycast has been captured by another node already.
        if (ray_cast->is_in_group(kCapturedGastRayCastGroupName) &&
            !has_captured_raycast(*ray_cast)) {
            continue;
        }

        // Check if the ray cast collides with this node.
        bool collides_with_node = false;
        Vector3 collision_point;
        Vector3 collision_normal;

        if (ray_cast->is_colliding()) {
            Node *collider = Object::cast_to<Node>(ray_cast->get_collider());
            if (collider != nullptr && node_path == collider->get_path()) {
                collides_with_node = true;

                collision_point = ray_cast->get_collision_point();
                collision_normal = ray_cast->get_collision_normal();
            }
        } else if (has_captured_raycast(*ray_cast) &&
                   colliding_raycast_paths[ray_cast_path]->press_in_progress) {
            // A press was in progress when the raycast 'move off' this node. Continue faking the
            // collision until the press is released.
            collision_point = colliding_raycast_paths[ray_cast_path]->collision_point;
            collision_normal = colliding_raycast_paths[ray_cast_path]->collision_normal;

            // Simulate collision and update collision_point accordingly.
            // Generate the plane defined by the collision normal and the collision point.
            auto *collision_plane = new Plane(collision_point, collision_normal);

            collides_with_node = calculate_raycast_plane_collision(*ray_cast, *collision_plane,
                                                                   &collision_point);
        }

        if (collides_with_node) {
            std::shared_ptr<CollisionInfo> collision_info =
                    has_captured_raycast(*ray_cast)
                    ? colliding_raycast_paths[ray_cast_path] : std::make_shared<CollisionInfo>();

            // Calculate the 2D collision point of the raycast on the Gast node.
            Vector2 relative_collision_point = get_relative_collision_point(collision_point);
            collision_info->press_in_progress = handle_ray_cast_input(ray_cast_path,
                                                                      relative_collision_point);
            collision_info->collision_normal = collision_normal;
            collision_info->collision_point = collision_point;

            // Add the raycast to the list of colliding raycasts and update its collision info.
            colliding_raycast_paths[ray_cast_path] = collision_info;

            // Add the raycast to the captured raycasts group.
            ray_cast->add_to_group(kCapturedGastRayCastGroupName);

            continue;
        }

        // Cleanup
        if (has_captured_raycast(*ray_cast)) {
            // Grab the last coordinates.
            Vector2 last_coordinate = get_relative_collision_point(
                    colliding_raycast_paths[ray_cast_path]->collision_point);
            if (colliding_raycast_paths[ray_cast_path]->press_in_progress) {
                // Fire a release event.
                GastManager::get_singleton_instance()->on_render_input_release(node_path,
                                                                               ray_cast_path,
                                                                               last_coordinate.x,
                                                                               last_coordinate.y);
            } else {
                // Fire a hover exit event.
                GastManager::get_singleton_instance()->on_render_input_hover(node_path,
                                                                             ray_cast_path,
                                                                             kInvalidCoordinate.x,
                                                                             kInvalidCoordinate.y);
            }

            // Remove the raycast from this node.
            colliding_raycast_paths.erase(ray_cast_path);

            // Remove the raycast from the captured raycasts group.
            ray_cast->remove_from_group(kCapturedGastRayCastGroupName);
        }
    }
}

bool GastNode::calculate_raycast_plane_collision(const RayCast &raycast, const Plane &plane,
                                                 Vector3 *collision_point) {
    return plane.intersects_ray(raycast.to_global(raycast.get_translation()),
                                raycast.to_global(raycast.get_cast_to()), collision_point);
}

bool
GastNode::handle_ray_cast_input(const String &ray_cast_path, Vector2 relative_collision_point) {
    Input *input = Input::get_singleton();
    String node_path = get_path();

    float x_percent = relative_collision_point.x;
    float y_percent = relative_collision_point.y;

    // Check for click actions
    String ray_cast_click_action = get_click_action_from_node_path(ray_cast_path);
    const bool press_in_progress = input->is_action_pressed(ray_cast_click_action);
    if (input->is_action_just_pressed(ray_cast_click_action)) {
        GastManager::get_singleton_instance()->on_render_input_press(node_path, ray_cast_path,
                                                                     x_percent, y_percent);
    } else if (input->is_action_just_released(ray_cast_click_action)) {
        GastManager::get_singleton_instance()->on_render_input_release(node_path, ray_cast_path,
                                                                       x_percent, y_percent);
    } else {
        GastManager::get_singleton_instance()->on_render_input_hover(node_path, ray_cast_path,
                                                                     x_percent, y_percent);
    }

    // Check for scrolling actions
    bool did_scroll = false;
    float horizontal_scroll_delta = 0;
    float vertical_scroll_delta = 0;

    // Horizontal scrolls
    String ray_cast_horizontal_left_scroll_action = get_horizontal_left_scroll_action_from_node_path(
            ray_cast_path);
    String ray_cast_horizontal_right_scroll_action = get_horizontal_right_scroll_action_from_node_path(
            ray_cast_path);
    if (input->is_action_pressed(ray_cast_horizontal_left_scroll_action)) {
        did_scroll = true;
        horizontal_scroll_delta = -input->get_action_strength(
                ray_cast_horizontal_left_scroll_action);
    } else if (input->is_action_pressed(ray_cast_horizontal_right_scroll_action)) {
        did_scroll = true;
        horizontal_scroll_delta = input->get_action_strength(
                ray_cast_horizontal_right_scroll_action);
    }

    // Vertical scrolls
    String ray_cast_vertical_down_scroll_action = get_vertical_down_scroll_action_from_node_path(
            ray_cast_path);
    String ray_cast_vertical_up_scroll_action = get_vertical_up_scroll_action_from_node_path(
            ray_cast_path);
    if (input->is_action_pressed(ray_cast_vertical_down_scroll_action)) {
        did_scroll = true;
        vertical_scroll_delta = -input->get_action_strength(ray_cast_vertical_down_scroll_action);
    } else if (input->is_action_pressed(ray_cast_vertical_up_scroll_action)) {
        did_scroll = true;
        vertical_scroll_delta = input->get_action_strength(ray_cast_vertical_up_scroll_action);
    }

    if (did_scroll) {
        GastManager::get_singleton_instance()->on_render_input_scroll(node_path, ray_cast_path,
                                                                      x_percent, y_percent,
                                                                      horizontal_scroll_delta,
                                                                      vertical_scroll_delta);
    }

    return press_in_progress;
}

Vector2 GastNode::get_relative_collision_point(Vector3 absolute_collision_point) {
    Vector3 local_point = to_local(absolute_collision_point);
    if (get_projection_mesh()->is_rectangular_projection_mesh()) {
        auto *rectangular_projection_mesh =
            dynamic_cast<RectangularProjectionMesh*>(get_projection_mesh());
        return rectangular_projection_mesh->get_relative_collision_point(local_point);
    } else {
        return kInvalidCoordinate;
    }
}

}  // namespace gast
