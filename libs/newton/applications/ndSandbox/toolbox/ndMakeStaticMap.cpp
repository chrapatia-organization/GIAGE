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
#include "ndDemoMesh.h"
#include "ndDemoEntity.h"
#include "ndLoadFbxMesh.h"
#include "ndPhysicsWorld.h"
#include "ndMakeStaticMap.h"
#include "ndDemoEntityManager.h"
#include "ndDemoSplinePathMesh.h"

ndBodyKinematic* BuildFloorBox(ndDemoEntityManager* const scene, const ndMatrix& location)
{
	ndPhysicsWorld* const world = scene->GetWorld();

	ndShapeInstance box(new ndShapeBox(200.0f, 1.0f, 200.f));
	ndMatrix uvMatrix(dGetIdentityMatrix());
	uvMatrix[0][0] *= 0.025f;
	uvMatrix[1][1] *= 0.025f;
	uvMatrix[2][2] *= 0.025f;
	ndDemoMesh* const geometry = new ndDemoMesh("box", scene->GetShaderCache(), &box, "marbleCheckBoard.tga", "marbleCheckBoard.tga", "marbleCheckBoard.tga", 1.0f, uvMatrix);

	ndDemoEntity* const entity = new ndDemoEntity(location, nullptr);
	entity->SetMesh(geometry, dGetIdentityMatrix());

	ndBodyDynamic* const body = new ndBodyDynamic();
	body->SetNotifyCallback(new ndDemoEntityNotify(scene, entity));
	body->SetMatrix(location);
	body->SetCollisionShape(box);

	world->AddBody(body);

	scene->AddEntity(entity);
	geometry->Release();
	return body;
}

ndBodyKinematic* BuildGridPlane(ndDemoEntityManager* const scene, ndInt32 grids, ndFloat32 gridSize, ndFloat32 perturbation)
{
	ndVector origin(-grids * gridSize * 0.5f, 0.0f, -grids * gridSize * 0.5f, 0.0f);

	ndArray<ndVector> points;
	for (ndInt32 iz = 0; iz <= grids; iz++)
	{
		ndFloat32 z0 = origin.m_z + iz * gridSize;
		for (ndInt32 ix = 0; ix <= grids; ix++)
		{
			ndFloat32 x0 = origin.m_x + ix * gridSize;
			points.PushBack(ndVector(x0, dGaussianRandom(perturbation), z0, 1.0f));
		}
	}

	ndMeshEffect meshEffect;
	meshEffect.BeginBuild();
	for (ndInt32 iz = 0; iz < grids; iz++)
	{ 
		for (ndInt32 ix = 0; ix < grids; ix++)
		{
			ndVector p0(points[(ix + 0) * (grids + 1) + iz + 0]);
			ndVector p1(points[(ix + 1) * (grids + 1) + iz + 0]);
			ndVector p2(points[(ix + 1) * (grids + 1) + iz + 1]);
			ndVector p3(points[(ix + 0) * (grids + 1) + iz + 1]);

			meshEffect.BeginBuildFace();
			meshEffect.AddPoint(p0.m_x, p0.m_y, p0.m_z);
			meshEffect.AddPoint(p1.m_x, p1.m_y, p1.m_z);
			meshEffect.AddPoint(p2.m_x, p2.m_y, p2.m_z);
			meshEffect.EndBuildFace();

			meshEffect.BeginBuildFace();
			meshEffect.AddPoint(p0.m_x, p0.m_y, p0.m_z);
			meshEffect.AddPoint(p2.m_x, p2.m_y, p2.m_z);
			meshEffect.AddPoint(p3.m_x, p3.m_y, p3.m_z);
			meshEffect.EndBuildFace();
		}
	}
	meshEffect.EndBuild(0.0f);

	ndPhysicsWorld* const world = scene->GetWorld();
	ndPolygonSoupBuilder meshBuilder;
	meshBuilder.Begin();

	ndInt32 vertexStride = meshEffect.GetVertexStrideInByte() / sizeof(ndFloat64);
	const ndFloat64* const vertexData = meshEffect.GetVertexPool();

	ndInt32 mark = meshEffect.IncLRU();

	ndVector face[16];
	ndPolyhedra::Iterator iter(meshEffect);
	for (iter.Begin(); iter; iter++)
	{
		ndEdge* const edge = &(*iter);
		if ((edge->m_incidentFace >= 0) && (edge->m_mark != mark))
		{
			ndInt32 count = 0;
			ndEdge* ptr = edge;
			do
			{
				ndInt32 i = ptr->m_incidentVertex * vertexStride;
				ndVector point(ndFloat32(vertexData[i + 0]), ndFloat32(vertexData[i + 1]), ndFloat32(vertexData[i + 2]), ndFloat32(1.0f));
				face[count] = point;
				count++;
				ptr->m_mark = mark;
				ptr = ptr->m_next;
			} while (ptr != edge);

			ndInt32 materialIndex = meshEffect.GetFaceMaterial(edge);
			meshBuilder.AddFace(&face[0].m_x, sizeof(ndVector), 3, materialIndex);
		}
	}
	meshBuilder.End(false);

	ndFloat32 uvScale = 1.0 / 16.0f;

	ndShapeInstance plane(new ndShapeStatic_bvh(meshBuilder));
	ndMatrix uvMatrix(dGetIdentityMatrix());
	uvMatrix[0][0] *= uvScale;
	uvMatrix[1][1] *= uvScale;
	uvMatrix[2][2] *= uvScale;
	ndDemoMesh* const geometry = new ndDemoMesh("plane", scene->GetShaderCache(), &plane, "marbleCheckBoard.tga", "marbleCheckBoard.tga", "marbleCheckBoard.tga", 1.0f, uvMatrix);

	ndMatrix matrix(dGetIdentityMatrix());
	ndDemoEntity* const entity = new ndDemoEntity(matrix, nullptr);
	entity->SetMesh(geometry, dGetIdentityMatrix());

	ndBodyDynamic* const body = new ndBodyDynamic();
	body->SetNotifyCallback(new ndDemoEntityNotify(scene, entity));
	body->SetMatrix(matrix);
	body->SetCollisionShape(plane);

	world->AddBody(body);

	scene->AddEntity(entity);
	geometry->Release();
	return body;
}

ndBodyKinematic* BuildFlatPlane(ndDemoEntityManager* const scene, bool optimized)
{
	ndPhysicsWorld* const world = scene->GetWorld();
	ndVector floor[] =
	{
		{ 200.0f, 0.0f,  200.0f, 1.0f },
		{ 200.0f, 0.0f, -200.0f, 1.0f },
		{ -200.0f, 0.0f, -200.0f, 1.0f },
		{ -200.0f, 0.0f,  200.0f, 1.0f },
	};
	ndInt32 index[][3] = { { 0, 1, 2 },{ 0, 2, 3 } };

	ndPolygonSoupBuilder meshBuilder;
	meshBuilder.Begin();
	meshBuilder.AddFaceIndirect(&floor[0].m_x, sizeof(ndVector), 31, &index[0][0], 3);
	meshBuilder.AddFaceIndirect(&floor[0].m_x, sizeof(ndVector), 31, &index[1][0], 3);
	meshBuilder.End(optimized);

	ndShapeInstance plane(new ndShapeStatic_bvh(meshBuilder));
	ndMatrix uvMatrix(dGetIdentityMatrix());
	uvMatrix[0][0] *= 0.025f;
	uvMatrix[1][1] *= 0.025f;
	uvMatrix[2][2] *= 0.025f;
	ndDemoMesh* const geometry = new ndDemoMesh("box", scene->GetShaderCache(), &plane, "marbleCheckBoard.tga", "marbleCheckBoard.tga", "marbleCheckBoard.tga", 1.0f, uvMatrix);

	ndMatrix matrix(dGetIdentityMatrix());
	ndDemoEntity* const entity = new ndDemoEntity(matrix, nullptr);
	entity->SetMesh(geometry, dGetIdentityMatrix());

	ndBodyDynamic* const body = new ndBodyDynamic();
	body->SetNotifyCallback(new ndDemoEntityNotify(scene, entity));
	body->SetMatrix(matrix);
	body->SetCollisionShape(plane);

	world->AddBody(body);

	scene->AddEntity(entity);
	geometry->Release();
	return body;
}

ndBodyKinematic* BuildStaticMesh(ndDemoEntityManager* const scene, const char* const meshName, bool optimized)
{
	fbxDemoEntity* const entity = scene->LoadFbxMesh(meshName);
	scene->AddEntity(entity);

	ndPolygonSoupBuilder meshBuilder;
	meshBuilder.Begin();
	
	ndInt32 stack = 1;
	ndMatrix matrixBuffer[1024];
	fbxDemoEntity* entBuffer[1024];
	
	entBuffer[0] = entity;
	matrixBuffer[0] = entity->GetCurrentMatrix().Inverse();

	while (stack)
	{
		stack--;
		fbxDemoEntity* const ent = entBuffer[stack];
		ndMatrix matrix (ent->GetCurrentMatrix() * matrixBuffer[stack]);
	
		if (ent->m_fbxMeshEffect)
		{
			ndInt32 vertexStride = ent->m_fbxMeshEffect->GetVertexStrideInByte() / sizeof (ndFloat64);
			const ndFloat64* const vertexData = ent->m_fbxMeshEffect->GetVertexPool();
	
			ndInt32 mark = ent->m_fbxMeshEffect->IncLRU();
			ndPolyhedra::Iterator iter(*ent->m_fbxMeshEffect);
			
			ndVector face[256];
			ndMatrix worldMatrix(ent->GetMeshMatrix() * matrix);
			for (iter.Begin(); iter; iter++)
			{
				ndEdge* const edge = &(*iter);
				if ((edge->m_incidentFace >= 0) && (edge->m_mark != mark))
				{
					ndInt32 count = 0;
					ndEdge* ptr = edge;
					do
					{
						ndInt32 i = ptr->m_incidentVertex * vertexStride;
						ndVector point(ndFloat32(vertexData[i + 0]), ndFloat32(vertexData[i + 1]), ndFloat32(vertexData[i + 2]), ndFloat32(1.0f));
						face[count] = worldMatrix.TransformVector(point);
						count++;
						ptr->m_mark = mark;
						ptr = ptr->m_next;
					} while (ptr != edge);
	
					ndInt32 materialIndex = ent->m_fbxMeshEffect->GetFaceMaterial(edge);
					meshBuilder.AddFace(&face[0].m_x, sizeof(ndVector), 3, materialIndex);
				}
			}
		}
	
		for (fbxDemoEntity* child = (fbxDemoEntity*)ent->GetChild(); child; child = (fbxDemoEntity*)child->GetSibling())
		{
			entBuffer[stack] = child;
			matrixBuffer[stack] = matrix;
			stack++;
		}
	}
	meshBuilder.End(optimized);
	ndShapeInstance shape(new ndShapeStatic_bvh(meshBuilder));

	ndMatrix matrix(entity->GetCurrentMatrix());
	ndBodyDynamic* const body = new ndBodyDynamic();
	body->SetNotifyCallback(new ndDemoEntityNotify(scene, entity));
	body->SetMatrix(matrix);
	body->SetCollisionShape(shape);
	scene->GetWorld()->AddBody(body);

	entity->CleanIntermediate();
	return body;
}

ndBodyKinematic* BuildPlayArena(ndDemoEntityManager* const scene)
{
	fbxDemoEntity* const entity = scene->LoadFbxMesh("playerarena.fbx");
	scene->AddEntity(entity);

	ndPolygonSoupBuilder meshBuilder;
	meshBuilder.Begin();

	ndInt32 stack = 1;
	ndMatrix matrixBuffer[1024];
	fbxDemoEntity* entBuffer[1024];

	entBuffer[0] = entity;
	matrixBuffer[0] = entity->GetCurrentMatrix().Inverse();

	while (stack)
	{
		stack--;
		fbxDemoEntity* const ent = entBuffer[stack];
		ndMatrix matrix(ent->GetCurrentMatrix() * matrixBuffer[stack]);

		if (ent->m_fbxMeshEffect)
		{
			ndInt32 vertexStride = ent->m_fbxMeshEffect->GetVertexStrideInByte() / sizeof(ndFloat64);
			const ndFloat64* const vertexData = ent->m_fbxMeshEffect->GetVertexPool();

			ndInt32 mark = ent->m_fbxMeshEffect->IncLRU();
			ndPolyhedra::Iterator iter(*ent->m_fbxMeshEffect);

			ndVector face[256];
			ndMatrix worldMatrix(ent->GetMeshMatrix() * matrix);
			for (iter.Begin(); iter; iter++)
			{
				ndEdge* const edge = &(*iter);
				if ((edge->m_incidentFace >= 0) && (edge->m_mark != mark))
				{
					ndInt32 count = 0;
					ndEdge* ptr = edge;
					do
					{
						ndInt32 i = ptr->m_incidentVertex * vertexStride;
						ndVector point(ndFloat32(vertexData[i + 0]), ndFloat32(vertexData[i + 1]), ndFloat32(vertexData[i + 2]), ndFloat32(1.0f));
						face[count] = worldMatrix.TransformVector(point);
						count++;
						ptr->m_mark = mark;
						ptr = ptr->m_next;
					} while (ptr != edge);

					ndInt32 materialIndex = ent->m_fbxMeshEffect->GetFaceMaterial(edge);
					meshBuilder.AddFace(&face[0].m_x, sizeof(ndVector), 3, materialIndex);
				}
			}
		}

		for (fbxDemoEntity* child = (fbxDemoEntity*)ent->GetChild(); child; child = (fbxDemoEntity*)child->GetSibling())
		{
			entBuffer[stack] = child;
			matrixBuffer[stack] = matrix;
			stack++;
		}
	}
	meshBuilder.End(true);
	ndShapeInstance shape(new ndShapeStatic_bvh(meshBuilder));

	ndMatrix matrix(entity->GetCurrentMatrix());
	ndBodyDynamic* const body = new ndBodyDynamic();
	body->SetNotifyCallback(new ndDemoEntityNotify(scene, entity));
	body->SetMatrix(matrix);
	body->SetCollisionShape(shape);
	scene->GetWorld()->AddBody(body);

	ndDemoEntity* const pivot0 = entity->Find("pivot1");
	ndDemoEntity* const pivot1 = entity->Find("pivot0");

	ndMatrix matrix0(pivot0->CalculateGlobalMatrix());
	ndMatrix matrix1(pivot1->CalculateGlobalMatrix());
	ndVector dir(matrix1.m_posit - matrix0.m_posit);
	ndFloat32 lenght = ndSqrt (dir.DotProduct(dir).GetScalar());

	const ndInt32 plankCount = 30;
	ndFloat32 sizex = 10.0f;
	ndFloat32 sizey = 0.25f;
	ndFloat32 sizez = lenght / plankCount;
	ndFloat32 deflection = 0.02f;

	matrix = matrix0;
	matrix.m_posit.m_y -= sizey * 0.5f;
	matrix.m_posit.m_z += sizez * 0.5f;

#if 1
	ndShapeInstance plankShape(new ndShapeBox(sizex, sizey, sizez + deflection ));

	ndFixSizeArray<ndBodyKinematic*, plankCount> array;
	for (ndInt32 i = 0; i < plankCount; i ++)
	{
		array.PushBack(CreateBody(scene, plankShape, matrix, 20.0f));
		matrix.m_posit.m_z += sizez;
	}

	for (ndInt32 i = 1; i < plankCount; i++)
	{
		ndBodyKinematic* body0 = array[i - 1];
		ndBodyKinematic* body1 = array[i];
		ndMatrix linkMatrix(body0->GetMatrix());
		linkMatrix.m_posit = ndVector::m_half * (body0->GetMatrix().m_posit + body1->GetMatrix().m_posit);
		linkMatrix.m_posit.m_y += sizey * 0.5f;
		ndMatrix matrix_0(linkMatrix);
		ndMatrix matrix_1(linkMatrix);
		matrix_0.m_posit.m_z += deflection  * 0.5f;
		matrix_1.m_posit.m_z -= deflection  * 0.5f;
		ndJointHinge* const hinge = new ndJointHinge(matrix_0, matrix_1, body0, body1);
		hinge->SetAsSpringDamper(true, 0.02f, 0.0f, 20.0f);
		scene->GetWorld()->AddJoint(hinge);
	}

	{
		ndBodyKinematic* body0 = array[0];
		ndBodyKinematic* body1 = body;
		ndMatrix linkMatrix(body0->GetMatrix());
		linkMatrix.m_posit = body0->GetMatrix().m_posit;
		linkMatrix.m_posit.m_z -= (sizez + deflection ) * 0.5f;
		linkMatrix.m_posit.m_y += sizey * 0.5f;
		ndMatrix matrix_0(linkMatrix);
		ndMatrix matrix_1(linkMatrix);
		matrix_0.m_posit.m_z += deflection  * 0.5f;
		matrix_1.m_posit.m_z -= deflection  * 0.5f;
		ndJointHinge* const hinge = new ndJointHinge(matrix_0, matrix_1, body0, body1);
		hinge->SetAsSpringDamper(true, 0.02f, 0.0f, 20.0f);
		scene->GetWorld()->AddJoint(hinge);
	}

	{
		ndBodyKinematic* body0 = array[plankCount-1];
		ndBodyKinematic* body1 = body;
		ndMatrix linkMatrix(body0->GetMatrix());
		linkMatrix.m_posit = body0->GetMatrix().m_posit;
		linkMatrix.m_posit.m_z += (sizez + deflection ) * 0.5f;
		linkMatrix.m_posit.m_y += sizey * 0.5f;
		ndMatrix matrix_0(linkMatrix);
		ndMatrix matrix_1(linkMatrix);
		matrix_0.m_posit.m_z += deflection  * 0.5f;
		matrix_1.m_posit.m_z -= deflection  * 0.5f;
		ndJointHinge* const hinge = new ndJointHinge(matrix_0, matrix_1, body0, body1);
		hinge->SetAsSpringDamper(true, 0.02f, 0.0f, 20.0f);
		scene->GetWorld()->AddJoint(hinge);
	}
#endif
	entity->CleanIntermediate();
	return body;
}

ndBodyKinematic* BuildSplineTrack(ndDemoEntityManager* const scene, const char* const meshName, bool optimized)
{
	ndFloat64 knots[] = { 0.0f, 1.0f / 3.0f, 2.0f / 3.0f, 1.0f };
	ndBigVector control[] =
	{
		ndBigVector(-16.0f, 1.0f, -10.0f, 1.0f),
		ndBigVector(-36.0f, 1.0f,   4.0f, 1.0f),
		ndBigVector(  4.0f, 1.0f,  15.0f, 1.0f),
		ndBigVector( 44.0f, 1.0f,   4.0f, 1.0f),
		ndBigVector(  4.0f, 1.0f, -22.0f, 1.0f),
		ndBigVector(-16.0f, 1.0f, -10.0f, 1.0f),
	};

	ndBezierSpline spline;
	spline.CreateFromKnotVectorAndControlPoints(3, sizeof(knots) / sizeof(knots[0]), knots, control);

	// recreate the spline from sample points at equally spaced distance 
	//ndPhysicsWorld* const world = scene->GetWorld();

	ndMatrix matrix(dGetIdentityMatrix());
	ndDemoSplinePathMesh* const splineMesh = new ndDemoSplinePathMesh(spline, scene->GetShaderCache(), 500);
	splineMesh->SetColor(ndVector(1.0f, 0.0f, 0.0f, 1.0f));
	ndDemoEntity* const splineEntity = new ndDemoEntity(matrix, nullptr);
	scene->AddEntity(splineEntity);
	splineEntity->SetMesh(splineMesh, dGetIdentityMatrix());
	splineMesh->SetVisible(true);
	splineMesh->Release();

	{
		// try different degrees
		//ndBigVector points[4];
		//points[0] = ndBigVector(-16.0f, 1.0f, -10.0f, 1.0f);
		//points[1] = ndBigVector(-36.0f, 1.0f, 4.0f, 1.0f);
		//points[2] = ndBigVector(4.0f, 1.0f, 15.0f, 1.0f);
		//points[3] = ndBigVector(44.0f, 1.0f, 4.0f, 1.0f);
		ndInt32 size = sizeof(control) / sizeof(control[0]);

		ndBigVector derivP0(control[1] - control[size-1]);
		//ndBigVector derivP1(control[0] - control[size - 2]);

		ndBezierSpline spline1;
		//spline1.GlobalCubicInterpolation(size, control, derivP0, derivP1);
		spline1.GlobalCubicInterpolation(size, control, derivP0, derivP0);

		//ndFloat64 u = (knots[1] + knots[2]) * 0.5f;
		//spline.InsertKnot(u);
		//spline.InsertKnot(u);
		//spline.InsertKnot(u);
		//spline.InsertKnot(u);
		//spline.InsertKnot(u);
		//spline.RemoveKnot(u, 1.0e-3f);
		ndDemoSplinePathMesh* const splineMesh1 = new ndDemoSplinePathMesh(spline1, scene->GetShaderCache(), 500);
		splineMesh1->SetColor(ndVector(0.0f, 1.0f, 0.0f, 1.0f));
		ndDemoEntity* const splineEntity1 = new ndDemoEntity(matrix, nullptr);
		scene->AddEntity(splineEntity1);
		splineEntity1->SetMesh(splineMesh1, dGetIdentityMatrix());
		splineMesh1->SetVisible(true);
		splineMesh1->Release();
	}


	return BuildStaticMesh(scene, meshName, optimized);
}
