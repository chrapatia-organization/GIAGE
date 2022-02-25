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

#ifndef _D_MESH_INTERFACE_H_
#define _D_MESH_INTERFACE_H_

#include "ndSandboxStdafx.h"
#include "ndOpenGlUtil.h"

class ndDemoEntity;
class ndDemoEntityManager;

class ndDemoSubMeshMaterial
{
	public:
	ndDemoSubMeshMaterial();
	~ndDemoSubMeshMaterial();

	glVector4 m_ambient;
	glVector4 m_diffuse;
	glVector4 m_specular;
	GLfloat m_opacity;
	GLfloat m_shiness;
	GLint m_textureHandle;
	char  m_textureName[32];
};

class ndDemoSubMesh
{
	public:
	ndDemoSubMesh();
	~ndDemoSubMesh();
	void SetOpacity(ndFloat32 opacity);

	ndDemoSubMeshMaterial m_material;
	ndInt32 m_indexCount;
	ndInt32 m_segmentStart;
	bool m_hasTranparency;
};

class ndDemoMeshInterface: public ndRefCounter<ndDemoMeshInterface>
{
	public:
	ndDemoMeshInterface();
	~ndDemoMeshInterface();
	const ndString& GetName () const;

	ndInt32 Release();
	bool GetVisible () const;
	void SetVisible (bool visibilityFlag);

	virtual ndDemoMeshInterface* Clone(ndDemoEntity* const) { dAssert(0); return nullptr; }

	virtual void Render (ndDemoEntityManager* const scene, const ndMatrix& modelMatrix) = 0;
	virtual void RenderTransparency(ndDemoEntityManager* const, const ndMatrix&) {}

	ndString m_name;
	bool m_isVisible;
};


#endif 


