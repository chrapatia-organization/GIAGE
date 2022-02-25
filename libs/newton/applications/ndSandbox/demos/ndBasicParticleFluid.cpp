/* Copyright (c) <2003-2021> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/

#include "ndSandboxStdafx.h"
#include "ndSkyBox.h"
#include "ndTargaToOpenGl.h"
#include "ndDemoMesh.h"
#include "ndDemoCamera.h"
#include "ndPhysicsUtils.h"
#include "ndPhysicsWorld.h"
#include "ndMakeStaticMap.h"
#include "ndDemoEntityManager.h"
#include "ndArchimedesBuoyancyVolume.h"


class ndIsoSurfaceMesh : public ndDemoMesh
{
	public:
	ndIsoSurfaceMesh(const ndShaderPrograms& shaderCache)
		:ndDemoMesh("isoSurface")
	{
		m_shader = shaderCache.m_diffuseEffect;

		ndDemoSubMesh* const segment = AddSubMesh();
		//segment->m_material.m_textureHandle = (GLuint)material;
		segment->m_material.m_textureHandle = (GLuint)1;

		segment->m_material.m_diffuse = ndVector(0.1f, 0.6f, 0.9f, 0.0f);
		segment->m_material.m_ambient = ndVector(0.1f, 0.6f, 0.9f, 0.0f);

		segment->SetOpacity(0.4f);
		segment->m_segmentStart = 0;
		segment->m_indexCount = 0;
		m_hasTransparency = true;
	}

	void UpdateBuffers(const ndArray<glPositionNormalUV>& points, const ndArray<ndInt32>& indexList)
	{
		//OptimizeForRender(points, indexList);
		OptimizeForRender(&points[0], points.GetCount(), &indexList[0], indexList.GetCount());

		ndDemoSubMesh& segment = GetFirst()->GetInfo();
		segment.m_indexCount = indexList.GetCount();
	}
};

class ndWaterVolumeEntity : public ndDemoEntity
{
	public:
	//ndWaterVolumeEntity(ndDemoEntityManager* const scene, const ndMatrix& location, const ndVector& size, ndBodySphFluid* const fluidBody, ndFloat32 radius)
		ndWaterVolumeEntity(ndDemoEntityManager* const scene, const ndMatrix& location, const ndVector&, ndBodySphFluid* const fluidBody, ndFloat32)
		:ndDemoEntity(location, nullptr)
		,m_fluidBody(fluidBody)
		,m_hasNewMesh(false)
	{
		ndShapeInstance box(new ndShapeBox(9.0f, 10.0f, 9.0f));
		ndMatrix uvMatrix(dGetIdentityMatrix());
		uvMatrix[0][0] *= 1.0f / 20.0f;
		uvMatrix[1][1] *= 1.0f / 10.0f;
		uvMatrix[2][2] *= 1.0f / 20.0f;
		uvMatrix.m_posit = ndVector(0.5f, 0.5f, 0.5f, 1.0f);

		ndDemoMesh* const geometry = new ndDemoMesh("fluidVolume", scene->GetShaderCache(), &box, "metal_30.tga", "metal_30.tga", "logo_php.tga", 0.5f, uvMatrix);
		SetMesh(geometry, dGetIdentityMatrix());

		scene->AddEntity(this);
		geometry->Release();

		m_isoSurfaceMesh0 = new ndIsoSurfaceMesh(scene->GetShaderCache());
		m_isoSurfaceMesh1 = new ndIsoSurfaceMesh(scene->GetShaderCache());
	}

	~ndWaterVolumeEntity()
	{
		ndScopeSpinLock lock(m_lock);
		m_isoSurfaceMesh0->Release();
		m_isoSurfaceMesh1->Release();
	}

	void Render(ndFloat32, ndDemoEntityManager* const scene, const ndMatrix&) const
	{
		// for now the mesh in is global space I need to fix that
		//ndMatrix nodeMatrix(m_matrix * matrix);

		ndMatrix nodeMatrix(dGetIdentityMatrix());
		//nodeMatrix.m_posit.m_y += 0.125f;
	
		// render the fluid;
		m_isoSurfaceMesh0->Render(scene, nodeMatrix);
		
		ndScopeSpinLock lock(m_lock);
		if (m_hasNewMesh)
		{
			m_hasNewMesh = false;
			m_isoSurfaceMesh1->UpdateBuffers(m_points, m_indexList);
			dSwap(m_isoSurfaceMesh0, m_isoSurfaceMesh1);
		}

		// render the cage;
		//ndDemoEntity::Render(timeStep, scene, matrix);
	}

	void UpdateIsoSuface(const ndIsoSurface& isoSurface)
	{
		if (isoSurface.GetVertexCount())
		{
			m_points.Resize(isoSurface.GetVertexCount());
			m_points.SetCount(isoSurface.GetVertexCount());
			const ndVector* const points = isoSurface.GetPoints();
			const ndVector* const normals = isoSurface.GetNormals();
			for (ndInt32 i = 0; i < isoSurface.GetVertexCount(); i++)
			{
				m_points[i].m_posit = glVector3(GLfloat(points[i].m_x), GLfloat(points[i].m_y), GLfloat(points[i].m_z));
				m_points[i].m_normal = glVector3(GLfloat(normals[i].m_x), GLfloat(normals[i].m_y), GLfloat(normals[i].m_z));
				m_points[i].m_uv.m_u = GLfloat(0.0f);
				m_points[i].m_uv.m_v = GLfloat(0.0f);
			}

			m_indexList.Resize(isoSurface.GetIndexCount());
			m_indexList.SetCount(isoSurface.GetIndexCount());
			const ndUnsigned64* const indexList = isoSurface.GetIndexList();
			for (ndInt32 i = 0; i < isoSurface.GetIndexCount(); i++)
			{
				m_indexList[i] = ndInt32(indexList[i]);
			}

			ndScopeSpinLock lock(m_lock);
			m_hasNewMesh = true;
		}
	}

	ndBodySphFluid* m_fluidBody;
	ndArray<ndInt32> m_indexList;
	ndArray<glPositionNormalUV> m_points;
	mutable bool m_hasNewMesh;
	mutable ndSpinLock m_lock;
	mutable ndIsoSurfaceMesh* m_isoSurfaceMesh0;
	mutable ndIsoSurfaceMesh* m_isoSurfaceMesh1;
};

class ndWaterVolumeCallback: public ndDemoEntityNotify
{
	public:
	ndWaterVolumeCallback(ndDemoEntityManager* const manager, ndDemoEntity* const entity)
		:ndDemoEntityNotify(manager, entity)
	{
	}

	void OnTransform(ndInt32, const ndMatrix&)
	{
		ndBodySphFluid* const fluid = GetBody()->GetAsBodySphFluid();
		dAssert(fluid);

		//fluid->GenerateIsoSurface(m_manager->GetWorld(), 0.25f);
		fluid->GenerateIsoSurface(m_manager->GetWorld());
		const ndIsoSurface& isoSurface = fluid->GetIsoSurface();

		ndWaterVolumeEntity* const entity = (ndWaterVolumeEntity*)GetUserData();
		entity->UpdateIsoSuface(isoSurface);
	}
};

static void AddWaterVolume(ndDemoEntityManager* const scene, const ndMatrix& location)
{
	ndPhysicsWorld* const world = scene->GetWorld();

	ndMatrix matrix(location);
	ndVector floor(FindFloor(*world, matrix.m_posit, 200.0f));
	matrix.m_posit = floor;
	matrix.m_posit.m_w = 1.0f;

	ndFloat32 diameter = 0.25f;
	ndBodySphFluid* const fluidObject = new ndBodySphFluid();
	ndWaterVolumeEntity* const entity = new ndWaterVolumeEntity(scene, matrix, ndVector(20.0f, 10.0f, 20.0f, 0.0f), fluidObject, diameter * 0.5f);

	fluidObject->SetNotifyCallback(new ndWaterVolumeCallback(scene, entity));
	fluidObject->SetMatrix(matrix);

	fluidObject->SetParticleRadius(diameter * 0.5f);

	//ndInt32 particleCountPerAxis = 32;
	//ndInt32 particleCountPerAxis = 25;
	ndInt32 particleCountPerAxis = 32;
	ndFloat32 spacing = diameter * 1.0f;

	ndFloat32 offset = spacing * particleCountPerAxis / 2.0f;
	ndVector origin(-offset, 1.0f, -offset, ndFloat32(0.0f));

	matrix.m_posit += origin;
	
	for (ndInt32 z = 0; z < particleCountPerAxis; z++)
	//for (ndInt32 z = 0; z < 1; z++)
	{
		for (ndInt32 y = 0; y < particleCountPerAxis; y++)
		//for (ndInt32 y = 0; y < 1; y++)
		{
			for (ndInt32 x = 0; x < particleCountPerAxis; x++)
			//for (ndInt32 x = 0; x < 1; x++)
			{
				ndVector posit (matrix.TransformVector(ndVector (x * spacing, y * spacing, z * spacing, ndFloat32 (1.0f))));
				fluidObject->AddParticle(0.1f, posit, ndVector::m_zero);

				//ndVector xxxx(ndVector::m_zero);
				//fluidObject->AddParticle(0.1f, xxxx, ndVector::m_zero);

				//posit += ndVector(0.01f, 0.01f, 0.01f, 0.0f);
				//fluidObject->AddParticle(0.1f, origin + posit, ndVector::m_zero);
			}
		}
	}

	world->AddBody(fluidObject);
}

void ndBasicParticleFluid (ndDemoEntityManager* const scene)
{
	// build a floor
	BuildFlatPlane(scene, true);

	ndMatrix location(dGetIdentityMatrix());

	// adding a water volume
	AddWaterVolume(scene, location);

	ndQuaternion rot;
	ndVector origin(-15.0f, 4.0f, 0.0f, 0.0f);
	scene->SetCameraMatrix(rot, origin);
}
