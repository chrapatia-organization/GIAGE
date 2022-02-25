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
#include "ndJointSlider.h"

D_CLASS_REFLECTION_IMPLEMENT_LOADER(ndJointSlider)

ndJointSlider::ndJointSlider(const ndMatrix& pinAndPivotFrame, ndBodyKinematic* const child, ndBodyKinematic* const parent)
	:ndJointBilateralConstraint(6, child, parent, pinAndPivotFrame)
	,m_posit(ndFloat32 (0.0f))
	,m_speed(ndFloat32(0.0f))
	,m_springK(ndFloat32(0.0f))
	,m_damperC(ndFloat32(0.0f))
	,m_minLimit(ndFloat32(0.0f))
	,m_maxLimit(ndFloat32(0.0f))
	,m_friction(ndFloat32(0.0f))
	,m_springDamperRegularizer(ndFloat32(0.1f))
	,m_hasLimits(false)
	,m_isSpringDamper(false)
{
}

ndJointSlider::ndJointSlider(const ndLoadSaveBase::ndLoadDescriptor& desc)
	:ndJointBilateralConstraint(ndLoadSaveBase::ndLoadDescriptor(desc))
	,m_posit(ndFloat32(0.0f))
	,m_speed(ndFloat32(0.0f))
	,m_springK(ndFloat32(0.0f))
	,m_damperC(ndFloat32(0.0f))
	,m_minLimit(ndFloat32(0.0f))
	,m_maxLimit(ndFloat32(0.0f))
	,m_friction(ndFloat32(0.0f))
	,m_springDamperRegularizer(ndFloat32(0.1f))
	,m_hasLimits(false)
	,m_isSpringDamper(false)
{
	const nd::TiXmlNode* const xmlNode = desc.m_rootNode;

	m_springK = xmlGetFloat(xmlNode, "springK");
	m_damperC = xmlGetFloat(xmlNode, "damperC");
	m_minLimit = xmlGetFloat(xmlNode, "minLimit");
	m_maxLimit = xmlGetFloat(xmlNode, "maxLimit");
	m_friction = xmlGetFloat(xmlNode, "friction");
	m_springDamperRegularizer = xmlGetFloat(xmlNode, "springDamperRegularizer");
	m_hasLimits = xmlGetInt(xmlNode, "hasLimits") ? true : false;
	m_isSpringDamper = xmlGetInt(xmlNode, "isSpringDamper") ? true : false;
}

ndJointSlider::~ndJointSlider()
{
}

ndFloat32 ndJointSlider::GetSpeed() const
{
	return m_speed;
}

ndFloat32 ndJointSlider::GetPosit() const
{
	return m_posit;
}

ndFloat32 ndJointSlider::GetFriction() const
{
	return m_friction;
}

void ndJointSlider::SetFriction(ndFloat32 friction)
{
	m_friction = dAbs(friction);
}

void ndJointSlider::EnableLimits(bool state, ndFloat32 minLimit, ndFloat32 maxLimit)
{
	m_hasLimits = state;
	dAssert(minLimit <= 0.0f);
	dAssert(maxLimit >= 0.0f);
	m_minLimit = minLimit;
	m_maxLimit = maxLimit;

	// adding one extra dof, this makes the mass matrix ill conditioned, 
	// but it could work with the direct solver
	m_maxDof = (m_isSpringDamper && m_hasLimits) ? 7 : 6;
}

void ndJointSlider::SetAsSpringDamper(bool state, ndFloat32 regularizer, ndFloat32 spring, ndFloat32 damper)
{
	m_springK = dAbs(spring);
	m_damperC = dAbs(damper);
	m_springDamperRegularizer = dClamp(regularizer, ndFloat32(1.0e-2f), ndFloat32(0.99f));
	m_isSpringDamper = state;

	// adding one extra dof, this makes the mass matrix ill conditioned, 
	// but it could work with the direct solver
	m_maxDof = (m_isSpringDamper && m_hasLimits) ? 7 : 6;
}

void ndJointSlider::SubmitConstraintLimits(ndConstraintDescritor& desc, const ndMatrix& matrix0, const ndMatrix& matrix1)
{
	if ((m_minLimit == ndFloat32 (0.0f)) && (m_maxLimit == ndFloat32(0.0f)))
	{
		AddLinearRowJacobian(desc, matrix0.m_posit, matrix1.m_posit, matrix1.m_front);
	}
	else 
	{
		ndFloat32 x = m_posit + m_speed * desc.m_timestep;
		if (x < m_minLimit)
		{
			ndVector p1(matrix1.m_posit + matrix1.m_front.Scale (m_minLimit));
			AddLinearRowJacobian(desc, matrix0.m_posit, p1, matrix1.m_front);
			const ndFloat32 stopAccel = GetMotorZeroAcceleration(desc);
			const ndFloat32 penetration = x - m_minLimit;
			const ndFloat32 recoveringAceel = -desc.m_invTimestep * D_SLIDER_PENETRATION_RECOVERY_SPEED * dMin(dAbs(penetration / D_SLIDER_PENETRATION_LIMIT), ndFloat32(1.0f));
			SetMotorAcceleration(desc, stopAccel - recoveringAceel);
			SetLowerFriction(desc, -m_friction); 
		}
		else if (x > m_maxLimit)
		{
			AddLinearRowJacobian(desc, matrix0.m_posit, matrix0.m_posit, matrix1.m_front);
			const ndFloat32 stopAccel = GetMotorZeroAcceleration(desc);
			const ndFloat32 penetration = x - m_maxLimit;
			const ndFloat32 recoveringAceel = desc.m_invTimestep * D_SLIDER_PENETRATION_RECOVERY_SPEED * dMin(dAbs(penetration / D_SLIDER_PENETRATION_LIMIT), ndFloat32(1.0f));
			SetMotorAcceleration(desc, stopAccel - recoveringAceel);
			SetHighFriction(desc, m_friction);
		}
		else if (m_friction > ndFloat32 (0.0f)) 
		{
			AddLinearRowJacobian(desc, matrix0.m_posit, matrix0.m_posit, matrix1.m_front);
			const ndFloat32 stopAccel = GetMotorZeroAcceleration(desc);
			SetMotorAcceleration(desc, stopAccel);
			SetLowerFriction(desc, -m_friction);
			SetHighFriction(desc, m_friction);
		}
	}
}

void ndJointSlider::SubmitConstraintLimitSpringDamper(ndConstraintDescritor& desc, const ndMatrix& matrix0, const ndMatrix& matrix1)
{
	// add spring damper row
	AddLinearRowJacobian(desc, matrix0.m_posit, matrix1.m_posit, matrix1.m_front);
	SetMassSpringDamperAcceleration(desc, m_springDamperRegularizer, m_springK, m_damperC);

	ndFloat32 x = m_posit + m_speed * desc.m_timestep;
	if (x < m_minLimit)
	{
		const ndVector p1(matrix1.m_posit + matrix1.m_front.Scale(m_minLimit));
		AddLinearRowJacobian(desc, matrix0.m_posit, p1, matrix1.m_front);
		const ndFloat32 stopAccel = GetMotorZeroAcceleration(desc);
		const ndFloat32 penetration = x - m_minLimit;
		const ndFloat32 recoveringAceel = -desc.m_invTimestep * D_SLIDER_PENETRATION_RECOVERY_SPEED * dMin(dAbs(penetration / D_SLIDER_PENETRATION_LIMIT), ndFloat32(1.0f));
		SetMotorAcceleration(desc, stopAccel - recoveringAceel);
		SetLowerFriction(desc, ndFloat32(0.0f));
	}
	else if (x > m_maxLimit)
	{
		const ndVector p1(matrix1.m_posit + matrix1.m_front.Scale(m_maxLimit));
		AddLinearRowJacobian(desc, matrix0.m_posit, p1, matrix1.m_front);
		const ndFloat32 stopAccel = GetMotorZeroAcceleration(desc);
		const ndFloat32 penetration = x - m_maxLimit;
		const ndFloat32 recoveringAceel = desc.m_invTimestep * D_SLIDER_PENETRATION_RECOVERY_SPEED * dMin(dAbs(penetration / D_SLIDER_PENETRATION_LIMIT), ndFloat32(1.0f));
		SetMotorAcceleration(desc, stopAccel - recoveringAceel);
		SetHighFriction(desc, ndFloat32(0.0f));
	}
}

void ndJointSlider::JacobianDerivative(ndConstraintDescritor& desc)
{
	ndMatrix matrix0;
	ndMatrix matrix1;

	// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
	CalculateGlobalMatrix(matrix0, matrix1);

	// calculate position and speed	
	const ndVector veloc0(m_body0->GetVelocityAtPoint(matrix0.m_posit));
	const ndVector veloc1(m_body1->GetVelocityAtPoint(matrix1.m_posit));

	const ndVector& pin = matrix1[0];
	const ndVector& p0 = matrix0.m_posit;
	const ndVector& p1 = matrix1.m_posit;
	const ndVector prel(p0 - p1);
	const ndVector vrel(veloc0 - veloc1);

	m_speed = vrel.DotProduct(matrix1.m_front).GetScalar();
	m_posit = prel.DotProduct(matrix1.m_front).GetScalar();
	const ndVector projectedPoint = p1 + pin.Scale(pin.DotProduct(prel).GetScalar());

	AddLinearRowJacobian(desc, p0, projectedPoint, matrix1[1]);
	AddLinearRowJacobian(desc, p0, projectedPoint, matrix1[2]);

	const ndFloat32 angle0 = CalculateAngle(matrix0.m_up, matrix1.m_up, matrix1.m_front);
	AddAngularRowJacobian(desc, matrix1.m_front, angle0);
	
	const ndFloat32 angle1 = CalculateAngle(matrix0.m_front, matrix1.m_front, matrix1.m_up);
	AddAngularRowJacobian(desc, matrix1.m_up, angle1);
	
	const ndFloat32 angle2 = CalculateAngle(matrix0.m_front, matrix1.m_front, matrix1.m_right);
	AddAngularRowJacobian(desc, matrix1.m_right, angle2);

	if (m_hasLimits)
	{
		if (m_isSpringDamper)
		{
			// spring damper with limits
			SubmitConstraintLimitSpringDamper(desc, matrix0, matrix1);
		}
		else
		{
			// only hard limits
			SubmitConstraintLimits(desc, matrix0, matrix1);
		}
	}
	else if (m_isSpringDamper)
	{
		// spring damper without limits
		AddLinearRowJacobian(desc, matrix0.m_posit, matrix1.m_posit, matrix1.m_front);
		SetMassSpringDamperAcceleration(desc, m_springDamperRegularizer, m_springK, m_damperC);
	}
	else if (m_friction > ndFloat32(0.0f))
	{
		AddLinearRowJacobian(desc, matrix0.m_posit, matrix0.m_posit, matrix1.m_front);
		const ndFloat32 stopAccel = GetMotorZeroAcceleration(desc);
		SetMotorAcceleration(desc, stopAccel);
		SetLowerFriction(desc, -m_friction);
		SetHighFriction(desc, m_friction);
	}
}

void ndJointSlider::Save(const ndLoadSaveBase::ndSaveDescriptor& desc) const
{
	nd::TiXmlElement* const childNode = new nd::TiXmlElement(ClassName());
	desc.m_rootNode->LinkEndChild(childNode);
	childNode->SetAttribute("hashId", desc.m_nodeNodeHash);
	ndJointBilateralConstraint::Save(ndLoadSaveBase::ndSaveDescriptor(desc, childNode));

	xmlSaveParam(childNode, "springK", m_springK);
	xmlSaveParam(childNode, "damperC", m_damperC);
	xmlSaveParam(childNode, "minLimit", m_minLimit);
	xmlSaveParam(childNode, "maxLimit", m_maxLimit);
	xmlSaveParam(childNode, "friction", m_friction);
	xmlSaveParam(childNode, "springDamperRegularizer", m_springDamperRegularizer);
	xmlSaveParam(childNode, "hasLimits", m_hasLimits ? 1 : 0);
	xmlSaveParam(childNode, "isSpringDamper", m_isSpringDamper ? 1 : 0);
}

