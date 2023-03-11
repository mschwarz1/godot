/**************************************************************************/
/*  csg.h                                                                 */
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

#ifndef CSG_H
#define CSG_H

#include "core/math/aabb.h"
#include "core/math/plane.h"
#include "core/math/transform_3d.h"
#include "core/math/vector2.h"
#include "core/math/vector3.h"
#include "core/object/ref_counted.h"
#include "core/templates/list.h"
#include "core/templates/oa_hash_map.h"
#include "core/templates/vector.h"
#include "scene/resources/material.h"

#include "thirdparty/glm/glm/ext/vector_int3.hpp"
#include "thirdparty/manifold/src/manifold/include/manifold.h"
#include "thirdparty/glm/glm/ext/vector_float3.hpp"

struct CSGBrush {
	struct Face {
		Vector3 vertices[3];
		Vector2 uvs[3];
		AABB aabb;
		bool smooth = false;
		bool invert = false;
		int material = 0;
	};
	enum class ManfifoldError {
		NoError,
		NonFiniteVertex,
		NotManifold,
		VertexOutOfBounds,
		PropertiesWrongLength,
		MissingPositionProperties,
		MergeVectorsDifferentLengths,
		MergeIndexOutOfBounds,
		TransformWrongLength,
		RunIndexWrongLength,
		FaceIDWrongLength,
	};
	CSGBrush() { }
	CSGBrush(CSGBrush &p_brush, const Transform3D &p_xform) {
		faces = p_brush.faces;
		materials = p_brush.materials;
		mesh_id_properties = p_brush.mesh_id_properties;
		mesh_id_triangle_property_indices = p_brush.mesh_id_triangle_property_indices;
		mesh_id_materials = p_brush.mesh_id_materials;
		for (int i = 0; i < faces.size(); i++) {
			for (int j = 0; j < 3; j++) {
				faces.write[i].vertices[j] = p_xform.xform(p_brush.faces[i].vertices[j]);
			}
		}
		create_manifold();
		_regen_face_aabbs();
	}

	Vector<Face> faces;
	Vector<Ref<Material>> materials;

	manifold::Manifold manifold;
	HashMap<int64_t, std::vector<float>> mesh_id_properties;
	HashMap<int64_t, std::vector<glm::ivec3>> mesh_id_triangle_property_indices;
	HashMap<int64_t, Vector<Ref<Material>>> mesh_id_materials;

	inline void _regen_face_aabbs() {
		for (int i = 0; i < faces.size(); i++) {
			faces.write[i].aabb = AABB();
			faces.write[i].aabb.position = faces[i].vertices[0];
			faces.write[i].aabb.expand_to(faces[i].vertices[1]);
			faces.write[i].aabb.expand_to(faces[i].vertices[2]);
		}
	}

	enum {
		MANIFOLD_PROPERTY_POSITION = 0,
		MANIFOLD_PROPERTY_NORMAL = MANIFOLD_PROPERTY_POSITION + 3,
		MANIFOLD_PROPERTY_INVERT,
		MANIFOLD_PROPERTY_SMOOTH_GROUP,
		MANIFOLD_PROPERTY_UV_X_0,
		//MANIFOLD_PROPERTY_UV_X_1,
		//MANIFOLD_PROPERTY_UV_X_2,
		MANIFOLD_PROPERTY_UV_Y_0,
		//MANIFOLD_PROPERTY_UV_Y_1,
		//MANIFOLD_PROPERTY_UV_Y_2,
		//MANIFOLD_PROPERTY_MATERIAL,
		MANIFOLD_PROPERTY_MAX
	};
	void create_manifold();
	void convert_manifold_to_brush();
	static void merge_manifold_properties(const HashMap<int64_t, std::vector<float>> &p_mesh_id_properties,
			const HashMap<int64_t, std::vector<glm::ivec3>> &p_mesh_id_triangle_property_indices,
			const HashMap<int64_t, Vector<Ref<Material>>> &p_mesh_id_materials,
			HashMap<int64_t, std::vector<float>> &r_mesh_id_properties,
			HashMap<int64_t, std::vector<glm::ivec3>> &r_mesh_id_triangle_property_indices,
			HashMap<int64_t, Vector<Ref<Material>>> &r_mesh_id_materials);

	// Create a brush from faces.
	void build_from_faces(const Vector<Vector3> &p_vertices, const Vector<Vector2> &p_uvs, const Vector<bool> &p_smooth, const Vector<Ref<Material>> &p_materials, const Vector<bool> &p_invert_faces);
};

#endif // CSG_H
