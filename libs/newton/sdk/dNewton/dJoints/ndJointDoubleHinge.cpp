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

#include "ndCoreStdafx.h"
#include "ndNewtonStdafx.h"
#include "ndJointDoubleHinge.h"

D_CLASS_REFLECTION_IMPLEMENT_LOADER(ndJointDoubleHinge)

ndJointDoubleHinge::ndJointDoubleHinge(const ndMatrix& pinAndPivotFrame, ndBodyKinematic* const child, ndBodyKinematic* const parent)
	:ndJointBilateralConstraint(6, child, parent, pinAndPivotFrame)
	,m_angle0(ndFloat32(0.0f))
	,m_omega0(ndFloat32(0.0f))
	,m_angle1(ndFloat32(0.0f))
	,m_omega1(ndFloat32(0.0f))
{
}

ndJointDoubleHinge::ndJointDoubleHinge(const ndLoadSaveBase::ndLoadDescriptor& desc)
	:ndJointBilateralConstraint(ndLoadSaveBase::ndLoadDescriptor(desc))
	,m_angle0(ndFloat32(0.0f))
	,m_omega0(ndFloat32(0.0f))
	,m_angle1(ndFloat32(0.0f))
	,m_omega1(ndFloat32(0.0f))
{
	//const nd::TiXmlNode* const xmlNode = desc.m_rootNode;
}

ndJointDoubleHinge::~ndJointDoubleHinge()
{
}

void ndJointDoubleHinge::Save(const ndLoadSaveBase::ndSaveDescriptor& desc) const
{
	nd::TiXmlElement* const childNode = new nd::TiXmlElement(ClassName());
	desc.m_rootNode->LinkEndChild(childNode);
	childNode->SetAttribute("hashId", desc.m_nodeNodeHash);
	ndJointBilateralConstraint::Save(ndLoadSaveBase::ndSaveDescriptor(desc, childNode));

	// since this joint is not used that much
	// for now double hinges do not have the hinge functionality
}

void ndJointDoubleHinge::JacobianDerivative(ndConstraintDescritor& desc)
{
	ndMatrix matrix0;
	ndMatrix matrix1;
	CalculateGlobalMatrix(matrix0, matrix1);

	//AddLinearRowJacobian(desc, matrix0.m_posit, matrix1.m_posit, matrix1[0]);
	//AddLinearRowJacobian(desc, matrix0.m_posit, matrix1.m_posit, matrix1[1]);
	//AddLinearRowJacobian(desc, matrix0.m_posit, matrix1.m_posit, matrix1[2]);

	const ndVector frontDir((matrix0.m_front - matrix1.m_up.Scale(matrix0.m_front.DotProduct(matrix1.m_up).GetScalar())).Normalize());
	const ndVector sideDir(frontDir.CrossProduct(matrix1.m_up));
	const ndFloat32 angle = CalculateAngle(matrix0.m_front, frontDir, sideDir);
	AddAngularRowJacobian(desc, sideDir, angle);

	// not happy with this method because it is a penalty system, 
	// but is hard for an first order integrator to prevent the side angle from drifting, 
	// even an implicit one with expanding the Jacobian partial derivatives still has a hard time
	// nullifying the gyro torque generate by the two angular velocities.
	const ndFloat32 alphaRollError = GetMotorZeroAcceleration(desc) + ndFloat32 (0.5f) * angle * desc.m_invTimestep * desc.m_invTimestep;
	SetMotorAcceleration(desc, alphaRollError);

	//// save the current joint Omega
	ndVector omega0(m_body0->GetOmega());
	ndVector omega1(m_body1->GetOmega());
	
	// calculate joint parameters, angles and omega
	const ndFloat32 deltaAngle0 = AnglesAdd(-CalculateAngle(matrix0.m_up, matrix1.m_up, frontDir), -m_angle0);
	m_angle0 += deltaAngle0;
	m_omega0 = frontDir.DotProduct(omega0 - omega1).GetScalar();

	const ndFloat32 deltaAngle1 = AnglesAdd(-CalculateAngle(frontDir, matrix1.m_front, matrix1.m_up), -m_angle1);
	m_angle1 += deltaAngle1;
	m_omega1 = matrix1.m_up.DotProduct(omega0 - omega1).GetScalar();

	//dTrace(("%f %f\n", m_jointAngle1 * dRadToDegree, m_omega1));
	
	//// two rows to restrict rotation around around the parent coordinate system
	//const ndFloat32 angle0 = CalculateAngle(matrix0.m_front, matrix1.m_front, matrix1.m_up);
	//AddAngularRowJacobian(desc, matrix1.m_up, angle0);
	//
	//const ndFloat32 angle1 = CalculateAngle(matrix0.m_front, matrix1.m_front, matrix1.m_right);
	//AddAngularRowJacobian(desc, matrix1.m_right, angle1);
}

