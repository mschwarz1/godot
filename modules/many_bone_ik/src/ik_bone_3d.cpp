/**************************************************************************/
/*  ik_bone_3d.cpp                                                        */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "ik_bone_3d.h"

#include "ik_kusudama_3d.h"
#include "many_bone_ik_3d.h"
#include "math/ik_node_3d.h"

void IKBone3D::set_bone_id(BoneId p_bone_id, Skeleton3D *p_skeleton) {
	ERR_FAIL_NULL(p_skeleton);
	bone_id = p_bone_id;
}

BoneId IKBone3D::get_bone_id() const {
	return bone_id;
}

void IKBone3D::set_parent(const Ref<IKBone3D> &p_parent) {
	ERR_FAIL_NULL(p_parent);
	parent = p_parent;
	if (parent.is_valid()) {
		parent->children.push_back(this);
		godot_skeleton_aligned_transform->set_parent(parent->godot_skeleton_aligned_transform);
		constraint_orientation_transform->set_parent(parent->godot_skeleton_aligned_transform);
		constraint_twist_transform->set_parent(parent->godot_skeleton_aligned_transform);
	}
}

void IKBone3D::update_default_bone_direction_transform(Skeleton3D *p_skeleton) {
	Vector3 child_centroid;
	if (children.is_empty()) {
		PackedInt32Array bone_children = p_skeleton->get_bone_children(bone_id);
		for (BoneId child_bone_idx : bone_children) {
			child_centroid += p_skeleton->get_bone_global_pose(child_bone_idx).origin;
		}
		child_centroid /= bone_children.size();
	} else {
		for (Ref<IKBone3D> ik_bone : children) {
			child_centroid += ik_bone->get_ik_transform()->get_global_transform().origin;
		}
		child_centroid /= children.size();
	}
	Vector3 godot_bone_origin = godot_skeleton_aligned_transform->get_global_transform().origin;
	child_centroid -= godot_bone_origin;
	if (Math::is_zero_approx(child_centroid.length_squared()) && parent.is_valid()) {
		child_centroid = parent->get_bone_direction_transform()->get_global_transform().basis.get_column(Vector3::AXIS_Y);
	} else if (Math::is_zero_approx(child_centroid.length_squared()) && parent.is_null()) {
		child_centroid = get_bone_direction_transform()->get_global_transform().basis.get_column(Vector3::AXIS_Y);
	}
	if (!Math::is_zero_approx(child_centroid.length_squared()) && (children.size() || p_skeleton->get_bone_children(bone_id).size())) {
		child_centroid.normalize();
		Vector3 bone_direction = bone_direction_transform->get_global_transform().basis.get_column(Vector3::AXIS_Y);
		bone_direction.normalize();
		bone_direction_transform->rotate_local_with_global(Quaternion(child_centroid, bone_direction));
	}
}

void IKBone3D::update_default_constraint_transform() {
	// TODO: If the user hasn't specified constraint transform, create it with a good guess.
	Ref<IKBone3D> parent_bone = get_parent();
	if (parent_bone.is_valid()) {
		Transform3D parent_bone_aligned_transform = parent_bone->get_ik_transform()->get_global_transform();
		parent_bone_aligned_transform.origin = get_bone_direction_transform()->get_global_transform().origin;
		constraint_orientation_transform->set_global_transform(parent_bone_aligned_transform);
	}
	Transform3D set_constraint_twist_transform = constraint_orientation_transform->get_global_transform();
	constraint_twist_transform->set_global_transform(set_constraint_twist_transform);
	// Orient the twist plane to the limit cone centroid (approximately).
	if (constraint.is_null()) {
		return;
	}
	TypedArray<IKLimitCone3D> cones = constraint->get_limit_cones();
	Vector3 direction;
	if (cones.size() == 0) { // if there are no limit cones, set the default twist orientation to align with the bone_direction
		direction = bone_direction_transform->get_global_transform().basis.get_column(Vector3::AXIS_Y);
	} else {
		for (int32_t cone_i = 0; cone_i < cones.size(); cone_i++) {
			Ref<IKLimitCone3D> cone = cones[cone_i];
			if (cone.is_null()) {
				break;
			}
			float weight = cone->get_radius() / Math_PI;
			direction += cone->get_control_point() * weight;
		}
		direction = direction.normalized();
		direction = constraint_orientation_transform->get_global_transform().xform(direction);
		direction -= constraint_orientation_transform->get_global_transform().origin;
	}
	Vector3 twist_axis = constraint_twist_transform->get_global_transform().basis.get_column(Vector3::AXIS_Y);
	Quaternion align_dir = Quaternion(twist_axis, direction);
	constraint_twist_transform->rotate_local_with_global(align_dir);
}

Ref<IKBone3D> IKBone3D::get_parent() const {
	return parent;
}

void IKBone3D::set_pin(const Ref<IKEffector3D> &p_pin) {
	ERR_FAIL_NULL(p_pin);
	pin = p_pin;
}

Ref<IKEffector3D> IKBone3D::get_pin() const {
	return pin;
}

void IKBone3D::set_pose(const Transform3D &p_transform) {
	godot_skeleton_aligned_transform->set_transform(p_transform);
}

Transform3D IKBone3D::get_pose() const {
	return godot_skeleton_aligned_transform->get_transform();
}

void IKBone3D::set_global_pose(const Transform3D &p_transform) {
	godot_skeleton_aligned_transform->set_global_transform(p_transform);
	Transform3D transform = constraint_orientation_transform->get_transform();
	transform.origin = godot_skeleton_aligned_transform->get_transform().origin;
	constraint_orientation_transform->set_transform(transform);
	constraint_orientation_transform->_propagate_transform_changed();
}

Transform3D IKBone3D::get_global_pose() const {
	return godot_skeleton_aligned_transform->get_global_transform();
}

Transform3D IKBone3D::get_bone_direction_global_pose() const {
	return bone_direction_transform->get_global_transform();
}

void IKBone3D::set_initial_pose(Skeleton3D *p_skeleton) {
	ERR_FAIL_NULL(p_skeleton);
	if (bone_id == -1) {
		return;
	}
	Transform3D bone_origin_to_parent_origin = p_skeleton->get_bone_pose(bone_id);
	set_pose(bone_origin_to_parent_origin);
}

void IKBone3D::set_skeleton_bone_pose(Skeleton3D *p_skeleton) {
	ERR_FAIL_NULL(p_skeleton);
	Transform3D bone_to_parent = get_pose();
	p_skeleton->set_bone_pose_position(bone_id, bone_to_parent.origin);
	p_skeleton->set_bone_pose_rotation(bone_id, bone_to_parent.basis.get_rotation_quaternion());
	// The ik solver doesn't modify scale, and if we do it'll be to orthonormalize which will break the underlying skeleton.
	// p_skeleton->set_bone_pose_scale(bone_id, bone_to_parent.basis.get_scale());
}

void IKBone3D::create_pin() {
	pin = Ref<IKEffector3D>(memnew(IKEffector3D(this)));
}

bool IKBone3D::is_pinned() const {
	return pin.is_valid();
}

void IKBone3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_pin"), &IKBone3D::get_pin);
	ClassDB::bind_method(D_METHOD("set_pin", "pin"), &IKBone3D::set_pin);
	ClassDB::bind_method(D_METHOD("is_pinned"), &IKBone3D::is_pinned);
	ClassDB::bind_method(D_METHOD("get_constraint"), &IKBone3D::get_constraint);
	ClassDB::bind_method(D_METHOD("get_constraint_orientation_transform"), &IKBone3D::get_constraint_orientation_transform);
	ClassDB::bind_method(D_METHOD("get_constraint_twist_transform"), &IKBone3D::get_constraint_twist_transform);
}

IKBone3D::IKBone3D(StringName p_bone, Skeleton3D *p_skeleton, const Ref<IKBone3D> &p_parent, Vector<Ref<IKEffectorTemplate3D>> &p_pins, float p_default_dampening,
		ManyBoneIK3D *p_many_bone_ik) {
	ERR_FAIL_NULL(p_skeleton);

	default_dampening = p_default_dampening;
	cos_half_dampen = cos(default_dampening / real_t(2.0));
	set_name(p_bone);
	bone_id = p_skeleton->find_bone(p_bone);
	if (p_parent.is_valid()) {
		set_parent(p_parent);
	}
	for (Ref<IKEffectorTemplate3D> elem : p_pins) {
		if (elem.is_null()) {
			continue;
		}
		if (elem->get_name() == p_bone && !elem->get_target_node().is_empty() && p_many_bone_ik && p_many_bone_ik->get_node_or_null(elem->get_target_node())) {
			create_pin();
			Ref<IKEffector3D> effector = get_pin();
			effector->set_target_node(p_skeleton, elem->get_target_node());
			effector->set_passthrough_factor(elem->get_passthrough_factor());
			effector->set_weight(elem->get_weight());
			effector->set_direction_priorities(elem->get_direction_priorities());
			break;
		}
	}
	bone_direction_transform->set_parent(godot_skeleton_aligned_transform);
}

float IKBone3D::get_cos_half_dampen() const {
	return cos_half_dampen;
}

void IKBone3D::set_cos_half_dampen(float p_cos_half_dampen) {
	cos_half_dampen = p_cos_half_dampen;
}

Ref<IKKusudama3D> IKBone3D::get_constraint() const {
	return constraint;
}

void IKBone3D::add_constraint(Ref<IKKusudama3D> p_constraint) {
	constraint = p_constraint;
}

Ref<IKNode3D> IKBone3D::get_ik_transform() {
	return godot_skeleton_aligned_transform;
}

Ref<IKNode3D> IKBone3D::get_constraint_orientation_transform() {
	return constraint_orientation_transform;
}

Ref<IKNode3D> IKBone3D::get_constraint_twist_transform() {
	return constraint_twist_transform;
}

void IKBone3D::set_constraint_orientation_transform(Ref<IKNode3D> p_transform) {
	constraint_orientation_transform = p_transform;
}

void IKBone3D::set_bone_direction_transform(Ref<IKNode3D> p_bone_direction) {
	bone_direction_transform = p_bone_direction;
}

Ref<IKNode3D> IKBone3D::get_bone_direction_transform() {
	return bone_direction_transform;
}

bool IKBone3D::is_orientationally_constrained() {
	if (get_constraint().is_null()) {
		return false;
	}
	return get_constraint()->is_orientationally_constrained();
}

bool IKBone3D::is_axially_constrained() {
	if (get_constraint().is_null()) {
		return false;
	}
	return get_constraint()->is_axially_constrained();
}
