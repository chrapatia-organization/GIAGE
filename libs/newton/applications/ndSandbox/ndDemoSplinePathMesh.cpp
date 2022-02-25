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
#include "ndDemoCamera.h"
#include "ndDemoEntity.h"
#include "ndTargaToOpenGl.h"
#include "ndDemoSplinePathMesh.h"
#include "ndDemoEntityManager.h"

ndDemoSplinePathMesh::ndDemoSplinePathMesh(const ndBezierSpline& curve, const ndShaderPrograms& shaderCache, ndInt32 resolution)
	:ndDemoMeshInterface()
	,m_curve(curve)
	,m_color(1.0f)
	,m_renderResolution(0)
{
	m_isVisible = false;
	m_shader = shaderCache.m_wireFrame;
	m_vertexBuffer = 0;
	m_vertextArrayBuffer = 0;
	m_shadeColorLocation = 0;
	m_projectionViewModelMatrixLocation = 0;
	SetRenderResolution(resolution);
}

ndDemoSplinePathMesh::~ndDemoSplinePathMesh()
{
	if (m_vertextArrayBuffer)
	{
		glDeleteBuffers(1, &m_vertexBuffer);
		glDeleteVertexArrays(1, &m_vertextArrayBuffer);
	}
}

ndInt32 ndDemoSplinePathMesh::GetRenderResolution() const
{
	return m_renderResolution;
}

void ndDemoSplinePathMesh::SetColor(const ndVector& color)
{
	m_color = color;
}

void ndDemoSplinePathMesh::SetRenderResolution(ndInt32 breaks)
{
	if (breaks != m_renderResolution)
	{
		m_renderResolution = breaks;
		if (m_vertextArrayBuffer)
		{
			glDeleteBuffers(1, &m_vertexBuffer);
			glDeleteVertexArrays(1, &m_vertextArrayBuffer);
		}

		ndFloat64 scale = 1.0f / m_renderResolution;
		ndArray<glVector3> points(m_renderResolution + 1);
		for (ndInt32 i = 0; i < m_renderResolution; i++)
		{
			ndBigVector p(m_curve.CurvePoint(i * scale));
			points.PushBack(glVector3(GLfloat(p.m_x), GLfloat(p.m_y), GLfloat(p.m_z)));
		}
		points.PushBack(points[0]);

		glGenVertexArrays(1, &m_vertextArrayBuffer);
		glBindVertexArray(m_vertextArrayBuffer);

		glGenBuffers(1, &m_vertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, (m_renderResolution + 1) * sizeof(glVector3), &points[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glVector3), (void*)0);

		glBindVertexArray(0);
		glDisableVertexAttribArray(0);

		glUseProgram(m_shader);

		m_shadeColorLocation = glGetUniformLocation(m_shader, "shadeColor");
		m_projectionViewModelMatrixLocation = glGetUniformLocation(m_shader, "projectionViewModelMatrix");

		glUseProgram(0);
	}
}

void ndDemoSplinePathMesh::Render(ndDemoEntityManager* const scene, const ndMatrix& modelMatrix)
{
	if (m_isVisible)
	{
		ndDemoCamera* const camera = scene->GetCamera();
		const glMatrix viewProjectionMatrix(modelMatrix * camera->GetViewMatrix() * camera->GetProjectionMatrix());

		const glVector4 color(m_color);

		glUseProgram(m_shader);
		glUniform4fv(m_shadeColorLocation, 1, &color[0]);
		glUniformMatrix4fv(m_projectionViewModelMatrixLocation, 1, false, &viewProjectionMatrix[0][0]);

		//ndMeshVector m_line[2];
		//glEnableClientState(GL_VERTEX_ARRAY);
		//glVertexPointer(3, GL_FLOAT, sizeof(ndMeshVector), m_line);
		//ndFloat64 scale = 1.0f / m_renderResolution;
		//ndBigVector p0(m_curve.CurvePoint(0.0f));
		//for (ndInt32 i = 1; i <= m_renderResolution; i++)
		//{
		//	ndBigVector p1(m_curve.CurvePoint(i * scale));
		//	m_line[0].m_x = GLfloat(p0.m_x);
		//	m_line[0].m_y = GLfloat(p0.m_y);
		//	m_line[0].m_z = GLfloat(p0.m_z);
		//	m_line[1].m_x = GLfloat(p1.m_x);
		//	m_line[1].m_y = GLfloat(p1.m_y);
		//	m_line[1].m_z = GLfloat(p1.m_z);
		//	glDrawArrays(GL_LINES, 0, 2);
		//	p0 = p1;
		//}
		//glDisableClientState(GL_VERTEX_ARRAY);

		glBindVertexArray(m_vertextArrayBuffer);
		glDrawArrays(GL_LINE_STRIP, 0, m_renderResolution + 1);
		glBindVertexArray(0);
		glUseProgram(0);
	}
}
