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

#include "ndSandboxStdafx.h"
#include "ndVehicleUI.h"
#include "ndDemoEntityManager.h"

//ndDemoMesh* CreateDialMesh(ndDemoEntityManager* const scene, const char* const texName)
//{
//	ndMeshEffect mesh;
//
//	dArray<ndMeshEffect::dMaterial>& materialArray = mesh.GetMaterials();
//	ndMeshEffect::dMaterial material;
//	strcpy(material.m_textureName, texName);
//	materialArray.PushBack(material);
//
//	ndFloat32 gageSize = 100.0f;
//	mesh.BeginBuild();
//	mesh.BeginBuildFace();
//	mesh.AddPoint(-gageSize, gageSize, 0.0f);
//	mesh.AddUV0(0.0f, 1.0f);
//	mesh.AddMaterial(0);
//
//	mesh.AddPoint(-gageSize, -gageSize, 0.0f);
//	mesh.AddUV0(0.0f, 0.0f);
//	mesh.AddMaterial(0);
//
//	mesh.AddPoint(gageSize, -gageSize, 0.0f);
//	mesh.AddUV0(1.0f, 0.0f);
//	mesh.AddMaterial(0);
//	mesh.EndBuildFace();
//
//	mesh.BeginBuildFace();
//	mesh.AddPoint(-gageSize, gageSize, 0.0f);
//	mesh.AddUV0(0.0f, 1.0f);
//	mesh.AddMaterial(0);
//
//	mesh.AddPoint(gageSize, -gageSize, 0.0f);
//	mesh.AddUV0(1.0f, 0.0f);
//	mesh.AddMaterial(0);
//
//	mesh.AddPoint(gageSize, gageSize, 0.0f);
//	mesh.AddUV0(1.0f, 1.0f);
//	mesh.AddMaterial(0);
//	mesh.EndBuildFace();
//
//	mesh.EndBuild(0.0f);
//	return new ndDemoMesh("dialMesh", &mesh, scene->GetShaderCache());
//}


const GLchar* ndVehicleUI::m_vertexShader =
	"in vec3 Position;\n"
	"in vec2 UV;\n"
	"out vec2 Frag_UV;\n"
	"out vec4 Frag_Color;\n"
	"uniform mat4 ProjMtx;\n"
	"uniform mat4 ModMtx;\n"
	"uniform float ptsize;\n"
	"uniform vec4 color;\n"
	"void main()\n"
	"{\n"
	"	Frag_UV = UV;\n"
	"	Frag_Color = color;\n"
	"	gl_Position = ProjMtx * ModMtx * vec4(Position.xy * ptsize,0.0,1.0);\n"
	"}\n"
;

const GLchar* ndVehicleUI::m_fragmentShader =
	"uniform sampler2D UIText;\n"
	"in vec2 Frag_UV;\n"
	"in vec4 Frag_Color;\n"
	"out vec4 Out_Color;\n"
	"void main()\n"
	"{\n"
	"	Out_Color = Frag_Color * texture(UIText, Frag_UV.st);\n"
	"}\n"
;

const GLchar* ndVehicleUI::m_vertexShaderWithVersion[2] = { "#version 330 core\n", m_vertexShader };
const GLchar* ndVehicleUI::m_fragmentShaderWithVersion[2] = { "#version 330 core\n", m_fragmentShader };

ndVehicleUI::ndVehicleUI()
	:ndClassAlloc()
	,m_shaderHandle(0)
	,m_vboDyn(0)
	,m_vboSta(0)
	,m_vaoDyn(0)
	,m_vaoSta(0)
	,m_iboDyn(0)
	,m_iboSta(0)
{
};

ndVehicleUI::~ndVehicleUI()
{
	if (m_shaderHandle)
	{
		glDeleteProgram(m_shaderHandle);
	}

	if (m_iboSta)
	{
		glDeleteBuffers(1, &m_iboSta);
	}

	if (m_vboSta)
	{
		glDeleteBuffers(1, &m_vboSta);
	}

	if (m_vaoSta)
	{
		glDeleteVertexArrays(1, &m_vaoSta);
	}
	if (m_iboDyn)
	{
		glDeleteBuffers(1, &m_iboDyn);
	}

	if (m_vboDyn)
	{
		glDeleteBuffers(1, &m_vboDyn);
	}

	if (m_vaoDyn)
	{
		glDeleteVertexArrays(1, &m_vaoDyn);
	}
};

void ndVehicleUI::CreateOrthoViewMatrix(ndDemoEntityManager* const uscene, ndFloat32 origin_x, const ndFloat32 origin_y, ndMatrix& projmatrix)
{
	ndFloat32 sizeX = (ndFloat32)(1.0f * uscene->GetWidth());
	ndFloat32 sizeY = (ndFloat32)(1.0f * uscene->GetHeight());
	ndFloat32 L = origin_x;
	ndFloat32 R = origin_x + sizeX;
	ndFloat32 T = origin_y;
	ndFloat32 B = origin_y + sizeY;
	projmatrix = ndMatrix({ 2.0f / (R - L), 0.0f,   0.0f, 0.0f },
	{ 0.0f,   2.0f / (T - B), 0.0f, 0.0f },
	{ 0.0f,   0.0f,          -1.0f, 0.0f },
	{ (R + L) / (L - R), (T + B) / (B - T), 0.0f, 1.0f });
}

void ndVehicleUI::CreateBufferUI()
{
	if (!m_vaoDyn)
	{
		m_shaderHandle = glCreateProgram();

		GLuint m_vertHandle = glCreateShader(GL_VERTEX_SHADER);
		GLuint m_fragHandle = glCreateShader(GL_FRAGMENT_SHADER);

		GLint Result = GL_FALSE;
		ndInt32 InfoLogLength = 0;

		glShaderSource(m_vertHandle, 2, m_vertexShaderWithVersion, NULL);
		glCompileShader(m_vertHandle);
		// Check Vertex Shader
		glGetShaderiv(m_vertHandle, GL_COMPILE_STATUS, &Result);
		glGetShaderiv(m_vertHandle, GL_INFO_LOG_LENGTH, &InfoLogLength);
		if (InfoLogLength > 0)
		{
			printf("Vertex shader error! \n");
			//	std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
			//	glGetShaderInfoLog(g_VertHandle3D, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
			//	printf("Vertex %s\n", &VertexShaderErrorMessage[0]);
		}
		glShaderSource(m_fragHandle, 2, m_fragmentShaderWithVersion, NULL);
		glCompileShader(m_fragHandle);

		// Check Fragment Shader
		glGetShaderiv(m_fragHandle, GL_COMPILE_STATUS, &Result);
		glGetShaderiv(m_fragHandle, GL_INFO_LOG_LENGTH, &InfoLogLength);
		if (InfoLogLength > 0)
		{
			printf("Fragment shader error! \n");
			//	std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
			//	glGetShaderInfoLog(g_FragHandle3D, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
			//	printf("Fragment %s\n", &FragmentShaderErrorMessage[0]);
		}

		glAttachShader(m_shaderHandle, m_vertHandle);
		glAttachShader(m_shaderHandle, m_fragHandle);

		glLinkProgram(m_shaderHandle);

		glDetachShader(m_shaderHandle, m_vertHandle);
		glDetachShader(m_shaderHandle, m_fragHandle);

		glDeleteShader(m_vertHandle);
		glDeleteShader(m_fragHandle);

		m_vertDyn[0].m_posit.m_x = -1.0f;
		m_vertDyn[0].m_posit.m_y = -1.0f;
		m_vertDyn[0].m_posit.m_z = 0.0f;
		m_vertDyn[0].m_uv.m_u = 0.0f;
		m_vertDyn[0].m_uv.m_v = 0.0f;
		//
		m_vertDyn[1].m_posit.m_x = -1.0f;
		m_vertDyn[1].m_posit.m_y = 1.0f;
		m_vertDyn[1].m_posit.m_z = 0.0f;
		m_vertDyn[1].m_uv.m_u = 0.0f;
		m_vertDyn[1].m_uv.m_v = 1.0f;

		m_vertDyn[2].m_posit.m_x = 1.0f;
		m_vertDyn[2].m_posit.m_y = 1.0f;
		m_vertDyn[2].m_posit.m_z = 0.0f;
		m_vertDyn[2].m_uv.m_u = -1.0f;
		m_vertDyn[2].m_uv.m_v = 1.0f;

		m_vertDyn[3].m_posit.m_x = 1.0f;
		m_vertDyn[3].m_posit.m_y = -1.0f;
		m_vertDyn[3].m_posit.m_z = 0.0f;
		m_vertDyn[3].m_uv.m_u = -1.0f;
		m_vertDyn[3].m_uv.m_v = 0.0f;

		m_indxDyn[0] = 0;
		m_indxDyn[1] = 1;
		m_indxDyn[2] = 2;
		m_indxDyn[3] = 2;
		m_indxDyn[4] = 3;
		m_indxDyn[5] = 0;

		glGenVertexArrays(1, &m_vaoDyn);
		glBindVertexArray(m_vaoDyn);

		glGenBuffers(1, &m_vboDyn);
		glBindBuffer(GL_ARRAY_BUFFER, m_vboDyn);
		glBufferData(GL_ARRAY_BUFFER, sizeof(m_vertDyn), &m_vertDyn[0], GL_DYNAMIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glPositionUV), (void*)0);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glPositionUV), (void*)(offsetof(glPositionUV, m_uv)));

		glGenBuffers(1, &m_iboDyn);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_iboDyn);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m_indxDyn), &m_indxDyn[0], GL_STATIC_DRAW);

		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(0);

		// Don't unbind this buffer, Let's it in opengl memory.
		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		// remove this buffer from memory because it is updated in runtime.
		// you need to bind this buffer at any render pass.
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(0);

		// Gear dynamic buffer
		memcpy(m_vertSta, m_vertDyn, sizeof(m_vertDyn));
		memcpy(m_indxSta, m_indxDyn, sizeof(m_indxDyn));
		//
		//
		glGenVertexArrays(1, &m_vaoSta);
		glBindVertexArray(m_vaoSta);
		//
		glGenBuffers(1, &m_vboSta);
		glBindBuffer(GL_ARRAY_BUFFER, m_vboSta);
		glBufferData(GL_ARRAY_BUFFER, sizeof(m_vertSta), &m_vertSta[0], GL_STATIC_DRAW);
		//
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glPositionUV), (void*)0);
		//
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glPositionUV), (void*)(offsetof(glPositionUV, m_uv)));

		glGenBuffers(1, &m_iboSta);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_iboSta);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m_indxSta), &m_indxSta[0], GL_STATIC_DRAW);
		//
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(0);
		//
		// Static buffer, Let's it in opengl memory.
		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		//glBindBuffer(GL_ARRAY_BUFFER, 0);
		//
		glBindVertexArray(0);
	}
};

void ndVehicleUI::RenderGageUI(ndDemoEntityManager* const uscene, const GLuint tex1, const ndFloat32 origin_x, const ndFloat32 origin_y, const ndFloat32 ptsize, ndFloat32 cparam, ndFloat32 minAngle, ndFloat32 maxAngle)
{
	if (m_vaoSta)
	{
		ndMatrix aprojm(dGetIdentityMatrix());
		CreateOrthoViewMatrix(uscene, origin_x, origin_y, aprojm);
		//
		minAngle *= -ndDegreeToRad;
		maxAngle *= -ndDegreeToRad;
		
		ndFloat32 angle = minAngle + (maxAngle - minAngle) * cparam;

		ndMatrix modm(dRollMatrix(-angle));
		glVector4 color(GLfloat(1.0f), GLfloat(1.0f), GLfloat(1.0f), GLfloat(1.0f));

		glMatrix glModm(modm);
		glMatrix glAprojm(aprojm);
		glUniformMatrix4fv(glGetUniformLocation(m_shaderHandle, "ProjMtx"), 1, GL_FALSE, &glAprojm[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(m_shaderHandle, "ModMtx"), 1, GL_FALSE, &glModm[0][0]);
		glUniform1f(glGetUniformLocation(m_shaderHandle, "ptsize"), GLfloat(ptsize));
		glUniform4fv(glGetUniformLocation(m_shaderHandle, "color"), 1, &color[0]);

		glBindVertexArray(m_vaoSta);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		// This buffer is already in the opengl memory.
		// Don't need to bind it again.
		//glBindBuffer(GL_ARRAY_BUFFER, m_vboSta);

		if (tex1)
		{
			glBindTexture(GL_TEXTURE_2D, tex1);
		}

		// This buffer is already in the memory.
		// Don't need to bind it again.
		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_iboSta);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(0);

		// Don't unbind this buffers from the opengl memory.
		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		//glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
};

void ndVehicleUI::RenderGearUI(ndDemoEntityManager* const uscene, const ndInt32 gearid, GLuint tex1, ndFloat32 origin_x, ndFloat32 origin_y, ndFloat32 ptsize)
{
	if (m_vaoDyn)
	{
		ndMatrix aprojm(dGetIdentityMatrix());
		CreateOrthoViewMatrix(uscene, origin_x, origin_y, aprojm);

		ndMatrix origin(dGetIdentityMatrix());
		origin[1][1] = -1.0f;
		origin.m_posit = ndVector(origin_x + ptsize * 1.9f, 50.0f, 0.0f, 1.0f);

		ndFloat32 uwith = 0.1f;
		ndFloat32 u0 = uwith * gearid;
		ndFloat32 u1 = u0 + uwith;
		ndFloat32 xy1 = 10.0f;

		glVector4 color;
		if (gearid == 0)
		{
			color = ndVector(1.0f, 0.5f, 0.0f, 1.0f);
		}
		else if (gearid == 1)
		{
			color = ndVector(1.0f, 1.0f, 0.0f, 1.0f);
		}
		else
		{
			color = ndVector(0.0f, 1.0f, 0.0f, 1.0f);
		}

		glMatrix glOrigin(origin);
		glMatrix glAprojm(aprojm);
		glUniformMatrix4fv(glGetUniformLocation(m_shaderHandle, "ProjMtx"), 1, GL_FALSE, &glAprojm[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(m_shaderHandle, "ModMtx"), 1, GL_FALSE, &glOrigin[0][0]);
		glUniform1f(glGetUniformLocation(m_shaderHandle, "ptsize"), GLfloat(xy1));
		glUniform4fv(glGetUniformLocation(m_shaderHandle, "color"), 1, &color[0]);

		glBindVertexArray(m_vaoDyn);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		//
		glBindBuffer(GL_ARRAY_BUFFER, m_vboDyn);

		m_vertDyn[0].m_uv.m_u = GLfloat(u0);
		m_vertDyn[1].m_uv.m_u = GLfloat(u0);

		m_vertDyn[2].m_uv.m_u = GLfloat(u1);
		m_vertDyn[3].m_uv.m_u = GLfloat(u1);

		// Bind and update the dynamic buffer uv data 
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(m_vertDyn), &m_vertDyn[0]);

		if (tex1)
		{
			glBindTexture(GL_TEXTURE_2D, tex1);
		}

		// This buffer is static, Don't need to bind it again.
		// The buffer is already bind in the opengl memory.
		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_iboDyn);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(0);

		// Don't unbind this buffer to let's it in opengl memory.
		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
};
