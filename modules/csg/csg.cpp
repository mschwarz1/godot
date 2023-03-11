/**************************************************************************/
/*  csg.cpp                                                               */
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

#include "csg.h"

#include "core/error/error_macros.h"
#include "core/math/color.h"
#include "core/math/geometry_2d.h"
#include "core/math/math_funcs.h"
#include "core/math/plane.h"
#include "core/math/vector2.h"
#include "core/math/vector3.h"
#include "core/templates/sort_array.h"
#include "core/variant/variant.h"
#include "scene/resources/material.h"
#include "scene/resources/mesh.h"
#include "scene/resources/mesh_data_tool.h"
#include "scene/resources/surface_tool.h"
#include "thirdparty/glm/glm/ext/vector_float4.hpp"
#include "thirdparty/manifold/src/manifold/include/manifold.h"
#include "thirdparty/manifold/src/utilities/include/public.h"

// CSGBrush

void CSGBrush::build_from_faces(const Vector<Vector3> &p_vertices, const Vector<Vector2> &p_uvs, const Vector<bool> &p_smooth, const Vector<Ref<Material>> &p_materials, const Vector<bool> &p_flip_faces) {
	faces.clear();

	int vc = p_vertices.size();

	ERR_FAIL_COND((vc % 3) != 0);

	const Vector3 *rv = p_vertices.ptr();
	int uvc = p_uvs.size();
	const Vector2 *ruv = p_uvs.ptr();
	int sc = p_smooth.size();
	const bool *rs = p_smooth.ptr();
	int mc = p_materials.size();
	const Ref<Material> *rm = p_materials.ptr();
	int ic = p_flip_faces.size();
	const bool *ri = p_flip_faces.ptr();

	HashMap<Ref<Material>, int> material_map;

	faces.resize(p_vertices.size() / 3);

	for (int i = 0; i < faces.size(); i++) {
		Face &f = faces.write[i];
		f.vertices[0] = rv[i * 3 + 0];
		f.vertices[1] = rv[i * 3 + 1];
		f.vertices[2] = rv[i * 3 + 2];

		if (uvc == vc) {
			f.uvs[0] = ruv[i * 3 + 0];
			f.uvs[1] = ruv[i * 3 + 1];
			f.uvs[2] = ruv[i * 3 + 2];
		}

		if (sc == vc / 3) {
			f.smooth = rs[i];
		} else {
			f.smooth = false;
		}

		if (ic == vc / 3) {
			f.invert = ri[i];
		} else {
			f.invert = false;
		}

		if (mc == vc / 3) {
			Ref<Material> mat = rm[i];
			if (mat.is_valid()) {
				HashMap<Ref<Material>, int>::ConstIterator E = material_map.find(mat);

				if (E) {
					f.material = E->value;
				} else {
					f.material = material_map.size();
					material_map[mat] = f.material;
				}

			} else {
				f.material = -1;
			}
		}
	}

	materials.resize(material_map.size());
	for (const KeyValue<Ref<Material>, int> &E : material_map) {
		materials.write[E.value] = E.key;
	}

	_regen_face_aabbs();
}

void CSGBrush::convert_manifold_to_brush() {
	manifold::MeshGL mesh = manifold.GetMeshGL();
	size_t triangle_count = mesh.NumTri();
	faces.resize(triangle_count);
	for (size_t triangle_i = 0; triangle_i < triangle_count; triangle_i++) {
		CSGBrush::Face &face = faces.write[triangle_i];
		for (int32_t face_vertex_i = 0; face_vertex_i < 3; face_vertex_i++) {
			constexpr int32_t order[3] = { 2, 1, 0 };
			size_t mesh_vertex = mesh.triVerts[triangle_i * 3 + order[face_vertex_i]];
			Vector3 position;
			for (int32_t i = 0; i < 3; i++) {
				position[i] = mesh.vertProperties[mesh_vertex * mesh.numProp + MANIFOLD_PROPERTY_POSITION + i];
			}
			face.vertices[face_vertex_i] = position;
			//glm::vec3 normal;
			//normal.x = mesh.vertProperties[vertex_i * mesh.numProp + MANIFOLD_PROPERTY_NORMAL + 0];
			//normal.y = mesh.vertProperties[vertex_i * mesh.numProp + MANIFOLD_PROPERTY_NORMAL + 1];
			//normal.z = mesh.vertProperties[vertex_i * mesh.numProp + MANIFOLD_PROPERTY_NORMAL + 2];
			//bool flat = Math::is_equal_approx(normal.x, normal.y) && Math::is_equal_approx(normal.x, normal.z);
			//face.smooth = !flat;
			//face.uvs[order[face_vertex_i]].x = mesh.vertProperties[vertex_i * mesh.numProp + MANIFOLD_PROPERTY_UV_X_0 + 0];
			//face.uvs[order[face_vertex_i]].y = mesh.vertProperties[vertex_i * mesh.numProp + MANIFOLD_PROPERTY_UV_Y_0 + 0];
			//face.invert = mesh.vertProperties[vertex_i * mesh.numProp + MANIFOLD_PROPERTY_INVERT];
			//face.smooth = mesh.vertProperties[vertex_i * mesh.numProp + MANIFOLD_PROPERTY_SMOOTH_GROUP];
			//size_t run_index = mesh.runIndex[vertex_i];
			// face.material = mesh.runIndex[triangle_i];
			// Vector<Ref<Material>> triangle_materials = mesh_id_materials[mesh.originalID[triangle_i]];
			// Ref<Material> mat = triangle_materials[run_index];
			// if (materials.find(mat) == -1) {
			// 	materials.push_back(mat);
			// }
		}
	}
	_regen_face_aabbs();
}

void CSGBrush::create_manifold() {
	if (!faces.size()) {
		manifold = manifold::Manifold();
		return;
	}
	Ref<SurfaceTool> st;
	st.instantiate();
	st->begin(Mesh::PRIMITIVE_TRIANGLES);
	for (const CSGBrush::Face &face : faces) {
		for (int32_t vertex_i = 0; vertex_i < 3; vertex_i++) {
			st->set_smooth_group(face.smooth);
			Vector2 uvs = face.uvs[vertex_i];
			st->set_uv(Vector2(uvs.x, uvs.y));
			int32_t mat_id = face.material;
			if (mat_id == -1 || mat_id >= materials.size()) {
				st->set_material(Ref<Material>());
			} else {
				st->set_material(materials[mat_id]);
			}
			st->add_vertex(face.vertices[vertex_i]);
		}
	}
	Ref<MeshDataTool> mdt;
	mdt.instantiate();
	mdt->create_from_surface(st->commit(), 0);
	manifold::MeshGL mesh;
	// Vector<Ref<Material>> triangle_material;
	// triangle_material.resize(face_count);
	// triangle_material.fill(Ref<Material>());
	mesh.numProp = MANIFOLD_PROPERTY_MAX;
	const int32_t VERTICES_IN_TRIANGLE = 3;
	int32_t number_of_triangles = mdt->get_face_count();
	mesh.triVerts.resize(number_of_triangles * VERTICES_IN_TRIANGLE);
	mesh.vertProperties.resize(number_of_triangles * mesh.numProp);
	//mesh.halfedgeTangent.resize(mesh.NumVert() * 4);
	HashMap<int32_t, Ref<Material>> vertex_material;
	//int32_t material_id = materials.find(mdt->get_material());
	for (int triangle_i = 0; triangle_i < mdt->get_face_count(); triangle_i++) {
		for (int32_t face_index_i = 0; face_index_i < VERTICES_IN_TRIANGLE; face_index_i++) {
			size_t vertex_index = mdt->get_face_vertex(triangle_i, face_index_i);
			mesh.triVerts[triangle_i * VERTICES_IN_TRIANGLE + MANIFOLD_PROPERTY_POSITION + face_index_i] = vertex_index;
		}
		SWAP(mesh.triVerts[triangle_i * VERTICES_IN_TRIANGLE + MANIFOLD_PROPERTY_POSITION + 0], mesh.triVerts[triangle_i * VERTICES_IN_TRIANGLE + MANIFOLD_PROPERTY_POSITION + 2]);
	}
	// Swap around indices, convert cw to ccw for the front face.
	for (int triangle_i = 0; triangle_i < number_of_triangles; triangle_i++) {
		for (int32_t face_index_i = 0; face_index_i < VERTICES_IN_TRIANGLE; face_index_i++) {
			int32_t vertex_i = mdt->get_face_vertex(triangle_i, face_index_i);
			Vector3 pos = mdt->get_vertex(vertex_i);
			mesh.vertProperties[vertex_i * mesh.numProp + face_index_i] = pos[face_index_i];
		}
	}
	for (int triangle_i = 0; triangle_i < number_of_triangles; triangle_i++) {
		for (int32_t face_index_i = 0; face_index_i < VERTICES_IN_TRIANGLE; face_index_i++) {
			int32_t vertex_i = mdt->get_face_vertex(triangle_i, face_index_i);
			Vector3 normal = -mdt->get_vertex_normal(vertex_i);
			mesh.vertProperties[triangle_i * mesh.numProp + MANIFOLD_PROPERTY_NORMAL + 0] = normal.x;
			mesh.vertProperties[triangle_i * mesh.numProp + MANIFOLD_PROPERTY_NORMAL + 1] = normal.y;
			mesh.vertProperties[triangle_i * mesh.numProp + MANIFOLD_PROPERTY_NORMAL + 2] = normal.z;
			//Plane tangent = mdt->get_vertex_tangent(vertex_i);
			//mesh.halfedgeTangent[vertex_i * 4 + 0] = tangent.normal.x;
			//mesh.halfedgeTangent[vertex_i * 4 + 1] = tangent.normal.y;
			//mesh.halfedgeTangent[vertex_i * 4 + 2] = tangent.normal.z;
			//mesh.halfedgeTangent[vertex_i * 4 + 3] = tangent.d;
			//mesh.vertProperties[vertex_i * mesh.numProp + MANIFOLD_PROPERTY_MATERIAL] = mdt->get_material();
			Vector2 uv = mdt->get_vertex_uv(vertex_i);
			mesh.vertProperties[triangle_i * mesh.numProp + MANIFOLD_PROPERTY_UV_X_0] = uv.x;
			mesh.vertProperties[triangle_i * mesh.numProp + MANIFOLD_PROPERTY_UV_Y_0] = uv.y;
			//if (material_id != -1) {
			//	vertex_material[triangle_i * face_count + face_index_i] = materials[material_id];
			//} else {
			//	vertex_material[triangle_i * face_count + face_index_i] = Ref<Material>();
			//}
			// triangle_material.write[triangle_i] = material_id;
		}
	}
	manifold = manifold::Manifold(mesh);
	manifold::Manifold::Error error = manifold.Status();
	if (error != manifold::Manifold::Error::NoError) {
		print_line(vformat("Cannot copy the other brush. %d", int(error)));
	}
}

void CSGBrush::merge_manifold_properties(const HashMap<int64_t, std::vector<float>> &p_mesh_id_properties,
		const HashMap<int64_t, std::vector<glm::ivec3>> &p_mesh_id_triangle_property_indices,
		const HashMap<int64_t, Vector<Ref<Material>>> &p_mesh_id_materials,
		HashMap<int64_t, std::vector<float>> &r_mesh_id_properties,
		HashMap<int64_t, std::vector<glm::ivec3>> &r_mesh_id_triangle_property_indices,
		HashMap<int64_t, Vector<Ref<Material>>> &r_mesh_id_materials) {
	for (const KeyValue<int64_t, std::vector<float>> &E : p_mesh_id_properties) {
		r_mesh_id_properties.operator[](E.key) = E.value;
	}
	for (const KeyValue<int64_t, std::vector<glm::ivec3>> &E : p_mesh_id_triangle_property_indices) {
		r_mesh_id_triangle_property_indices.operator[](E.key) = E.value;
	}
	for (const KeyValue<int64_t, Vector<Ref<Material>>> &E : p_mesh_id_materials) {
		r_mesh_id_materials.operator[](E.key) = E.value;
	}
}
