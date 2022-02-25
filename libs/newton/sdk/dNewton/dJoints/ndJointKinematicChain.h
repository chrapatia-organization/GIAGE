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

#ifndef __ND_JOINT_TWO_BODY_IK_H__
#define __ND_JOINT_TWO_BODY_IK_H__

#include "ndNewtonStdafx.h"
#include "ndJointBallAndSocket.h"

class ndJointKinematicChain: public ndJointBilateralConstraint
{
	public:
	D_CLASS_REFLECTION(ndJointKinematicChain);
	D_NEWTON_API ndJointKinematicChain(const ndLoadSaveBase::ndLoadDescriptor& desc);
	D_NEWTON_API ndJointKinematicChain(const ndVector& globalHipPivot, const ndMatrix& globalPinAndPivot, ndBodyKinematic* const child, ndBodyKinematic* const parent);
	D_NEWTON_API virtual ~ndJointKinematicChain();

	D_NEWTON_API void SetTargetLocalMatrix(const ndMatrix& matrix);
	D_NEWTON_API void SetTargetGlobalMatrix(const ndMatrix& matrix);

	protected:
	D_NEWTON_API void JacobianDerivative(ndConstraintDescritor& desc);
	D_NEWTON_API void Save(const ndLoadSaveBase::ndSaveDescriptor& desc) const;
	D_NEWTON_API void DebugJoint(ndConstraintDebugCallback& debugCallback) const;

	void SubmitAngularAxis(const ndMatrix& matrix0, const ndMatrix& matrix1, ndConstraintDescritor& desc);
	void SubmitAngularAxisCartesianApproximation(const ndMatrix& matrix0, const ndMatrix& matrix1, ndConstraintDescritor& desc);
	

	ndMatrix m_baseFrame;
	ndVector m_hipPivot;

	ndFloat32 m_angle;
	ndFloat32 m_minAngle;
	ndFloat32 m_maxAngle;
	ndFloat32 m_angularSpring;
	ndFloat32 m_angularDamper;
	ndFloat32 m_angularRegularizer;

	ndFloat32 m_maxDist;
	ndFloat32 m_linearSpring;
	ndFloat32 m_linearDamper;
	ndFloat32 m_linearRegularizer;
};


#endif 

