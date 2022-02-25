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

#ifndef _D_MAKE_STATIC_MAP_H_
#define _D_MAKE_STATIC_MAP_H_

#include "ndSandboxStdafx.h"
class ndDemoEntityManager;

ndBodyKinematic* BuildPlayArena(ndDemoEntityManager* const scene);
ndBodyKinematic* BuildFloorBox(ndDemoEntityManager* const scene, const ndMatrix& location);
ndBodyKinematic* BuildFlatPlane(ndDemoEntityManager* const scene, bool optimized);
ndBodyKinematic* BuildStaticMesh(ndDemoEntityManager* const scene, const char* const meshName, bool optimized);
ndBodyKinematic* BuildSplineTrack(ndDemoEntityManager* const scene, const char* const meshName, bool optimized);
ndBodyKinematic* BuildGridPlane(ndDemoEntityManager* const scene, ndInt32 grids, ndFloat32 gridSize, ndFloat32 perturbation);

#endif