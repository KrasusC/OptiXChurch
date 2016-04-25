#include "model.h"
#include <optixu/optixu_math_stream_namespace.h>

#include <OptiXMesh.h>

#include <iostream>
#include <cstdlib>

#include <memory.h>

using namespace optix;

Model::Model(std::string &objfilename, optix::Material matl, AccelDescriptor& accel_desc, optix::TextureSampler projectedTexSamp, optix::Program intersectProgram,
	optix::Context context, optix::GeometryGroup inGG) : m_geom_group(inGG), m_this_owns_geom_group(false)
{
	static bool printedPermissions = false;
	if (!printedPermissions) {
		if (objfilename.find("Fish_OBJ_PPM") != std::string::npos) {
			std::cout << "\nModels and textures copyright Toru Miyazawa, Toucan Corporation. Used by permission.\n";
			std::cout << "http://toucan.web.infoseek.co.jp/3DCG/3ds/FishModelsE.html\n\n";
			printedPermissions = true;
		}
	}

	//m_species = FishMonger_t::FindSpecies(objfilename);

	// std::cerr << "Found name: " << m_species->name << '\n';
	GeometryInstance GI;
	if (m_geom_group.get() == 0) {
		std::cerr << "Loading " << objfilename << '\n';
		m_this_owns_geom_group = true;

		m_geom_group = context->createGeometryGroup();
		OptiXMesh FishLoader(context, m_geom_group, matl, accel_desc);

		float m[16] = {
			0, -1, 0, 0,
			0, 0, 1, 0,
			-1, 0, 0, 0,
			0, 0, 0, 1
		};
		Matrix4x4 Rot(m);
		Matrix4x4 XForm = Rot;
		//XForm = Matrix4x4::scale(make_float3(1.0f / m_species->sizeInModel)) * XForm;
		XForm = Matrix4x4::translate(make_float3(0, 0, 0)) * XForm;
		XForm = Matrix4x4::rotate(1, make_float3(1, 0, 0)) * XForm;
		//XForm = Matrix4x4::scale(make_float3(m_species->lengthMeters)) * XForm;

		FishLoader.setLoadingTransform(XForm);
		if (intersectProgram)
			FishLoader.setDefaultIntersectionProgram(intersectProgram);

		FishLoader.loadBegin_Geometry(objfilename);
		FishLoader.loadFinish_Materials();

		m_aabb = FishLoader.getSceneBBox();

		// Set the material properties that differ between the fish and the other scene elements
		for (unsigned int i = 0; i < m_geom_group->getChildCount(); ++i) {
			GI = m_geom_group->getChild(i);
			if (projectedTexSamp)
				GI["caustic_map"]->setTextureSampler(projectedTexSamp);
			GI["diffuse_map_scale"]->setFloat(1.0f);
			GI["emission_color"]->setFloat(0);
			GI["Kr"]->setFloat(0);
		}
	}
	else {
		GI = m_geom_group->getChild(0);
	}

	m_geom = GI->getGeometry();
	m_vert_buff = m_geom["vertex_buffer"]->getBuffer();
	m_vert_buff->getSize(m_num_verts); // Query number of vertices in the buffer

	m_norm_buff = m_geom["normal_buffer"]->getBuffer();
	m_norm_buff->getSize(m_num_norms); // Query number of normals in the buffer, which doesn't match the number of vertices

	m_vindices_buff = m_geom["vindex_buffer"]->getBuffer();
	m_vindices_buff->getSize(m_num_tris);

	m_nindices_buff = m_geom["nindex_buffer"]->getBuffer();

	m_tran = context->createTransform();
	m_tran->setChild(m_geom_group);
}
