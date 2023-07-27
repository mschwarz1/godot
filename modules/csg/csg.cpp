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
	manifold::Manifold::Error error = manifold.Status();
	if (error != manifold::Manifold::Error::NoError) {
		print_line(vformat("Cannot convert brush error %d", int(error)));
		_regen_face_aabbs();
		return;
	}
	manifold::MeshGL mesh = manifold.GetMeshGL();

	Vector<int32_t> indices;
	const int32_t VERTICES_IN_TRIANGLE = 3;
	indices.resize(mesh.NumTri() * VERTICES_IN_TRIANGLE);
	indices.fill(-1);

	for (int triangle_i = 0; triangle_i < mesh.NumTri(); ++triangle_i) {
		for (int vert_i = 0; vert_i < VERTICES_IN_TRIANGLE; ++vert_i) {
			indices.write[triangle_i * VERTICES_IN_TRIANGLE + vert_i] = mesh.triVerts[triangle_i * VERTICES_IN_TRIANGLE + vert_i];
		}
	}

	for (int i = 0; i < mesh.NumTri(); ++i) {
		SWAP(indices.write[i * VERTICES_IN_TRIANGLE], indices.write[i * VERTICES_IN_TRIANGLE + 1]);
	}

	faces.clear();
	for (int triangle_i = 0; triangle_i < mesh.NumTri(); triangle_i++) {
		CSGBrush::Face face;
		for (int32_t face_index_i = 0; face_index_i < VERTICES_IN_TRIANGLE; face_index_i++) {
			int32_t clockwise_vertex_i = indices[triangle_i * mesh.NumTri() + face_index_i];
			Vector3 pos;
			pos.x = mesh.vertProperties[clockwise_vertex_i * mesh.numProp + MANIFOLD_PROPERTY_POSITION + 0];
			pos.y = mesh.vertProperties[clockwise_vertex_i * mesh.numProp + MANIFOLD_PROPERTY_POSITION + 1];
			pos.z = mesh.vertProperties[clockwise_vertex_i * mesh.numProp + MANIFOLD_PROPERTY_POSITION + 2];
			face.vertices[clockwise_vertex_i] = pos;
		}
		faces.push_back(face);
	}
	_regen_face_aabbs();
}

void CSGBrush::create_manifold() {
	if (!faces.size()) {
		manifold = manifold::Manifold();
		return;
	}
	// Ref<SurfaceTool> st;
	// st.instantiate();
	// st->begin(Mesh::PRIMITIVE_TRIANGLES);
	// for (const CSGBrush::Face &face : faces) {
	// 	for (int32_t vertex_i = 2; vertex_i >= 0; vertex_i--) {
	// 		st->set_smooth_group(face.smooth);
	// 		int32_t mat_id = face.material;
	// 		if (mat_id == -1 || mat_id >= materials.size()) {
	// 			st->set_material(Ref<Material>());
	// 		} else {
	// 			st->set_material(materials[mat_id]);
	// 		}
	// 		st->add_vertex(face.vertices[vertex_i]);
	// 	}
	// }
	// st->index();
	// Ref<MeshDataTool> mdt;
	// mdt.instantiate();
	// mdt->create_from_surface(st->commit(), 0);
	// mesh = manifold::MeshGL();
	// mesh.numProp = MANIFOLD_PROPERTY_MAX;
	// const int32_t VERTICES_IN_TRIANGLE = 3;
	// int32_t number_of_triangles = mdt->get_face_count();
	// Vector<int32_t> indices;
	// indices.resize(number_of_triangles * VERTICES_IN_TRIANGLE);
	// indices.fill(-1);

	// for (int i = 0; i < number_of_triangles; ++i) {
	// 	for (int j = 0; j < VERTICES_IN_TRIANGLE; ++j) {
	// 		indices.write[i * VERTICES_IN_TRIANGLE + j] = mdt->get_face_vertex(i, j);
	// 	}
	// }

	// for (int i = 0; i < number_of_triangles; ++i) {
	// 	SWAP(indices.write[i * VERTICES_IN_TRIANGLE], indices.write[i * VERTICES_IN_TRIANGLE + 1]);
	// }
	// mesh.triVerts.resize(number_of_triangles * MANIFOLD_PROPERTY_MAX);

	// for (int i = 0; i < indices.size(); ++i) {
	// 	mesh.triVerts[i] = indices[i];
	// }

	// mesh.vertProperties.resize(number_of_triangles * VERTICES_IN_TRIANGLE * mesh.numProp);
	// for (int triangle_i = 0; triangle_i < number_of_triangles; triangle_i++) {
	// 	for (int32_t face_index_i = 0; face_index_i < VERTICES_IN_TRIANGLE; face_index_i++) {
	// 		int32_t counter_clockwise_vertex = indices[triangle_i * VERTICES_IN_TRIANGLE + face_index_i];
	// 		Vector3 pos = mdt->get_vertex(counter_clockwise_vertex);
	// 		mesh.vertProperties[triangle_i * VERTICES_IN_TRIANGLE * mesh.numProp + face_index_i * mesh.numProp + MANIFOLD_PROPERTY_POSITION] = pos.x;
	// 		mesh.vertProperties[triangle_i * VERTICES_IN_TRIANGLE * mesh.numProp + face_index_i * mesh.numProp + MANIFOLD_PROPERTY_POSITION + 1] = pos.y;
	// 		mesh.vertProperties[triangle_i * VERTICES_IN_TRIANGLE * mesh.numProp + face_index_i * mesh.numProp + MANIFOLD_PROPERTY_POSITION + 2] = pos.z;
	// 	}
	// }

	manifold = manifold::Manifold::Cube();
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
