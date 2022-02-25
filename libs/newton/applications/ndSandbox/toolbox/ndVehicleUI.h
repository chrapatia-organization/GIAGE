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

// this vehicle UI class was implemented by Dave Gravel,
// so sole some of the Open Gl errors with legacy glBegin/GlEnd 
// operation with deprecated since OpenGl 3.3 
// Thank you very much Dave

#ifndef __ndVehicleUI_H__
#define __ndVehicleUI_H__

#include "ndSandboxStdafx.h"
#include "ndOpenGlUtil.h"

class ndDemoEntityManager;

class ndVehicleUI: public ndClassAlloc
{
	public:

	ndVehicleUI();
	~ndVehicleUI();
	
	void CreateBufferUI();
	void CreateOrthoViewMatrix(ndDemoEntityManager* const uscene, ndFloat32 origin_x, const ndFloat32 origin_y, ndMatrix& projmatrix);
	void RenderGearUI(ndDemoEntityManager* const uscene, const ndInt32 gearid, GLuint tex1, ndFloat32 origin_x, ndFloat32 origin_y, ndFloat32 ptsize);
	void RenderGageUI(ndDemoEntityManager* const uscene, const GLuint tex1, const ndFloat32 origin_x, const ndFloat32 origin_y, const ndFloat32 ptsize, ndFloat32 cparam, ndFloat32 minAngle, ndFloat32 maxAngle);

	
	GLuint m_shaderHandle;
	static const GLchar* m_vertexShader;
	static const GLchar* m_fragmentShader;
	static const GLchar* m_vertexShaderWithVersion[2];
	static const GLchar* m_fragmentShaderWithVersion[2];

	private:
	GLuint m_vboDyn;
	GLuint m_vboSta;
	GLuint m_vaoDyn;
	GLuint m_vaoSta;
	GLuint m_iboDyn;
	GLuint m_iboSta;
	glPositionUV m_vertDyn[4];
	glPositionUV m_vertSta[4];
	ndUnsigned32 m_indxDyn[6];
	ndUnsigned32 m_indxSta[6];
};

#endif