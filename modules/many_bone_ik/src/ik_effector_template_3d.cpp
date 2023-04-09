/**************************************************************************/
/*  ik_effector_template_3d.cpp                                           */
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

#include "ik_effector_template_3d.h"

#include "many_bone_ik_3d.h"

void IKEffectorTemplate3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_target_node"), &IKEffectorTemplate3D::get_target_node);
	ClassDB::bind_method(D_METHOD("set_target_node", "target_node"), &IKEffectorTemplate3D::set_target_node);

	ClassDB::bind_method(D_METHOD("get_passthrough_factor"), &IKEffectorTemplate3D::get_passthrough_factor);
	ClassDB::bind_method(D_METHOD("set_passthrough_factor", "passthrough_factor"), &IKEffectorTemplate3D::set_passthrough_factor);

	ClassDB::bind_method(D_METHOD("get_weight"), &IKEffectorTemplate3D::get_weight);
	ClassDB::bind_method(D_METHOD("set_weight", "weight"), &IKEffectorTemplate3D::set_weight);

	ClassDB::bind_method(D_METHOD("get_direction_priorities"), &IKEffectorTemplate3D::get_direction_priorities);
	ClassDB::bind_method(D_METHOD("set_direction_priorities", "direction_priorities"), &IKEffectorTemplate3D::set_direction_priorities);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "passthrough_factor"), "set_passthrough_factor", "get_passthrough_factor");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "weight"), "set_weight", "get_weight");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "direction_priorities"), "set_direction_priorities", "get_direction_priorities");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target_node"), "set_target_node", "get_target_node");
}

NodePath IKEffectorTemplate3D::get_target_node() const {
	return target_node;
}

void IKEffectorTemplate3D::set_target_node(NodePath p_node_path) {
	target_node = p_node_path;
}

float IKEffectorTemplate3D::get_passthrough_factor() const {
	return passthrough_factor;
}

void IKEffectorTemplate3D::set_passthrough_factor(float p_passthrough_factor) {
	passthrough_factor = p_passthrough_factor;
}

IKEffectorTemplate3D::IKEffectorTemplate3D() {
}
