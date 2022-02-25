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
#include "ndJointDryRollingFriction.h"

D_CLASS_REFLECTION_IMPLEMENT_LOADER(ndJointDryRollingFriction)

ndJointDryRollingFriction::ndJointDryRollingFriction(ndBodyKinematic* const body0, ndBodyKinematic* const body1, ndFloat32 coefficient)
	:ndJointBilateralConstraint(1, body0, body1, dGetIdentityMatrix())
	,m_coefficient(dClamp (coefficient, ndFloat32(0.0f), ndFloat32 (1.0f)))
	,m_contactTrail(ndFloat32 (0.1f))
{
	ndMatrix matrix(body0->GetMatrix());
	CalculateLocalMatrix(matrix, m_localMatrix0, m_localMatrix1);

	SetSolverModel(m_jointIterativeSoft);
}

ndJointDryRollingFriction::ndJointDryRollingFriction(const ndLoadSaveBase::ndLoadDescriptor& desc)
	:ndJointBilateralConstraint(ndLoadSaveBase::ndLoadDescriptor(desc))
{
	const nd::TiXmlNode* const xmlNode = desc.m_rootNode;

	m_coefficient = xmlGetFloat(xmlNode, "coefficient");
	m_contactTrail = xmlGetFloat(xmlNode, "contactTrail");
}


ndJointDryRollingFriction::~ndJointDryRollingFriction()
{
}

// rolling friction works as follow: the idealization of the contact of a spherical object 
// with a another surface is a point that pass by the center of the sphere.
// in most cases this is enough to model the collision but in insufficient for modeling 
// the rolling friction. In reality contact with the sphere with the other surface is not 
// a point but a contact patch. A contact patch has the property the it generates a fix 
// constant rolling torque that opposes the movement of the sphere.
// we can model this torque by adding a clamped torque aligned to the instantaneously axis 
// of rotation of the ball. and with a magnitude of the stopping angular acceleration.
//void ndJointDryRollingFriction::SubmitConstraints (dFloat timestep, int threadIndex)
void ndJointDryRollingFriction::JacobianDerivative(ndConstraintDescritor& desc)
{
	const ndBodyKinematic::ndContactMap& contactMap = m_body0->GetContactMap();

	ndFloat32 maxForce = ndFloat32 (0.0f);
	ndBodyKinematic::ndContactMap::Iterator it(contactMap);
	for (it.Begin(); it; it++)
	{
		const ndContact* const contact = *it;
		if (contact->IsActive())
		{
			const ndContactPointList& contactPoints = contact->GetContactPoints();
			for (ndContactPointList::ndNode* node = contactPoints.GetFirst(); node; node = node->GetNext())
			{
				const ndForceImpactPair& normalForce = node->GetInfo().m_normal_Force;
				ndFloat32 force = normalForce.GetInitialGuess();
				maxForce = dMax(force, maxForce);
			}
		}
	}
	
	if (maxForce > ndFloat32 (0.0f))
	{
		ndVector omega(m_body0->GetOmega());

		ndFloat32 omegaMag = omega.DotProduct(omega).GetScalar();
		if (omegaMag > ndFloat32(0.1f * 0.1f))
		{
			// tell newton to used this the friction of the omega vector to apply the rolling friction
			ndVector pin(omega.Normalize());

			AddAngularRowJacobian(desc, pin, ndFloat32(0.0f));

			ndFloat32 stopAccel = GetMotorZeroAcceleration(desc);
			SetMotorAcceleration(desc, stopAccel);
			
			ndFloat32 torqueFriction = maxForce * m_coefficient * m_contactTrail;
			SetLowerFriction(desc, -torqueFriction);
			SetHighFriction(desc, torqueFriction);
		}
		else
		{
			// when omega is too low cheat a little bit and damp the omega directly
			omega = omega.Scale(0.5f);
			m_body0->SetOmega(omega);
		}
	}
}

void ndJointDryRollingFriction::Save(const ndLoadSaveBase::ndSaveDescriptor& desc) const
{
	nd::TiXmlElement* const childNode = new nd::TiXmlElement(ClassName());
	desc.m_rootNode->LinkEndChild(childNode);
	childNode->SetAttribute("hashId", desc.m_nodeNodeHash);
	ndJointBilateralConstraint::Save(ndLoadSaveBase::ndSaveDescriptor(desc, childNode));

	xmlSaveParam(childNode, "coefficient", m_coefficient);
	xmlSaveParam(childNode, "contactTrail", m_contactTrail);
}


