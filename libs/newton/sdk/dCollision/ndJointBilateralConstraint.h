/* Copyright (c) <2003-2021> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __ND_JOINT_BILATERAL_CONSTRAINT_H__
#define __ND_JOINT_BILATERAL_CONSTRAINT_H__

#include "ndCollisionStdafx.h"
#include "ndJointList.h"
#include "ndConstraint.h"
#include "ndBodyKinematic.h"

#define DG_BILATERAL_CONTRAINT_DOF	8

enum ndJointBilateralSolverModel
{
	m_jointIterativeSoft,
	m_jointkinematicOpenLoop,
	m_jointkinematicCloseLoop,
	m_jointkinematicAttachment,
	m_jointModesCount
};

D_MSV_NEWTON_ALIGN_32
class ndJointBilateralConstraint : public ndConstraint
{
	public:
	D_CLASS_REFLECTION(ndJointBilateralConstraint);
	D_COLLISION_API ndJointBilateralConstraint(const ndLoadSaveBase::ndLoadDescriptor& desc);
	D_COLLISION_API ndJointBilateralConstraint(ndInt32 maxDof, ndBodyKinematic* const body0, ndBodyKinematic* const body1, const ndMatrix& globalMatrix);
	D_COLLISION_API virtual ~ndJointBilateralConstraint();

	virtual ndBodyKinematic* GetBody0() const;
	virtual ndBodyKinematic* GetBody1() const;
	void ReplaceSentinel(ndBodyKinematic* const sentinel);

	virtual ndUnsigned32 GetRowsCount() const;
	virtual ndJointBilateralConstraint* GetAsBilateral() { return this; }
	virtual void JacobianDerivative(ndConstraintDescritor& desc);
	virtual ndJointBilateralSolverModel GetSolverModel() const;
	virtual void SetSolverModel(ndJointBilateralSolverModel model);

	D_COLLISION_API ndFloat32 CalculateAngle(const ndVector& planeDir, const ndVector& cosDir, const ndVector& sinDir) const;
	D_COLLISION_API virtual void JointAccelerations(ndJointAccelerationDecriptor* const desc);
	D_COLLISION_API void CalculateLocalMatrix(const ndMatrix& pinsAndPivotFrame, ndMatrix& localMatrix0, ndMatrix& localMatrix1) const;
	D_COLLISION_API void AddAngularRowJacobian(ndConstraintDescritor& desc, const ndVector& dir, ndFloat32 relAngle);
	D_COLLISION_API void AddLinearRowJacobian(ndConstraintDescritor& desc, const ndVector& pivot0, const ndVector& pivot1, const ndVector& dir);

	D_COLLISION_API virtual void Save(const ndLoadSaveBase::ndSaveDescriptor& desc) const;
	D_COLLISION_API virtual void DebugJoint(ndConstraintDebugCallback& debugCallback) const;
	D_COLLISION_API ndFloat32 CalculateSpringDamperAcceleration(ndFloat32 dt, ndFloat32 ks, ndFloat32 x, ndFloat32 kd, ndFloat32 v) const;
	D_COLLISION_API void SetMassSpringDamperAcceleration(ndConstraintDescritor& desc, ndFloat32 regularizer, ndFloat32 spring, ndFloat32 damper);
	
	const ndMatrix& GetLocalMatrix0() const;
	const ndMatrix& GetLocalMatrix1() const;

	ndVector GetForceBody0() const;
	ndVector GetTorqueBody0() const;
	ndVector GetForceBody1() const;
	ndVector GetTorqueBody1() const;

	bool IsBilateral() const;
	bool IsCollidable() const;
	void SetCollidable(bool state);
	void SetSkeletonFlag(bool flag);
	void CalculateGlobalMatrix(ndMatrix& matrix0, ndMatrix& matrix1) const;
	ndFloat32 GetMotorZeroAcceleration(ndConstraintDescritor& desc) const;
	void SetHighFriction(ndConstraintDescritor& desc, ndFloat32 friction);
	void SetLowerFriction(ndConstraintDescritor& desc, ndFloat32 friction);
	void SetMotorAcceleration(ndConstraintDescritor& desc, ndFloat32 acceleration);
	ndFloat32 GetDiagonalRegularizer(const ndConstraintDescritor& desc) const;
	void SetDiagonalRegularizer(ndConstraintDescritor& desc, ndFloat32 stiffness);

	bool IsSkeleton() const;

	protected:
	ndMatrix m_localMatrix0;
	ndMatrix m_localMatrix1;
	ndVector m_forceBody0;
	ndVector m_torqueBody0;
	ndVector m_forceBody1;
	ndVector m_torqueBody1;

	ndVector m_r0[DG_BILATERAL_CONTRAINT_DOF];
	ndVector m_r1[DG_BILATERAL_CONTRAINT_DOF];
	ndForceImpactPair m_jointForce[DG_BILATERAL_CONTRAINT_DOF];
	ndFloat32 m_motorAcceleration[DG_BILATERAL_CONTRAINT_DOF];
	ndBodyKinematic* m_body0;
	ndBodyKinematic* m_body1;
	ndJointList::ndNode* m_worldNode;
	ndJointList::ndNode* m_body0Node;
	ndJointList::ndNode* m_body1Node;

	ndFloat32 m_defualtDiagonalRegularizer;
	ndUnsigned32 m_maxDof			: 6;
	ndUnsigned32 m_mark0				: 1;
	ndUnsigned32 m_mark1				: 1;
	ndUnsigned32 m_isInSkeleton		: 1;
	ndUnsigned32 m_enableCollision	: 1;
	ndInt8 m_rowIsMotor;
	ndJointBilateralSolverModel m_solverModel;

	friend class ndWorld;
	friend class ndDynamicsUpdate;
	friend class ndSkeletonContainer;
	friend class ndDynamicsUpdateSoa;
	friend class ndDynamicsUpdateAvx2;
	friend class ndDynamicsUpdateOpencl;
};

inline ndJointBilateralSolverModel ndJointBilateralConstraint::GetSolverModel() const
{
	return m_solverModel;
}

inline void ndJointBilateralConstraint::SetSolverModel(ndJointBilateralSolverModel model)
{
	dAssert(model < m_jointModesCount);
	dAssert(model >= m_jointIterativeSoft);
	m_solverModel = dClamp(model, m_jointIterativeSoft, m_jointModesCount);
}

inline ndUnsigned32 ndJointBilateralConstraint::GetRowsCount() const
{
	return m_maxDof;
}

inline void ndJointBilateralConstraint::CalculateGlobalMatrix(ndMatrix& matrix0, ndMatrix& matrix1) const
{
	matrix0 = m_localMatrix0 * m_body0->GetMatrix();
	matrix1 = m_localMatrix1 * m_body1->GetMatrix();
}

inline ndBodyKinematic* ndJointBilateralConstraint::GetBody0() const
{
	return m_body0;
}

inline ndBodyKinematic* ndJointBilateralConstraint::GetBody1() const
{
	return m_body1;
}

inline const ndMatrix& ndJointBilateralConstraint::GetLocalMatrix0() const
{
	return m_localMatrix0;
}

inline const ndMatrix& ndJointBilateralConstraint::GetLocalMatrix1() const
{
	return m_localMatrix1;
}

inline ndFloat32 ndJointBilateralConstraint::GetMotorZeroAcceleration(ndConstraintDescritor& desc) const
{
	const ndInt32 index = desc.m_rowsCount - 1;
	dAssert(index >= 0);
	dAssert(index < ndInt32(m_maxDof));
	return desc.m_zeroRowAcceleration[index];
}

inline void ndJointBilateralConstraint::SetMotorAcceleration(ndConstraintDescritor& desc, ndFloat32 acceleration)
{
	const ndInt32 index = desc.m_rowsCount - 1;
	dAssert(index >= 0);
	dAssert(index < ndInt32(m_maxDof));
	m_rowIsMotor |= (1 << index);
	desc.m_flags[index] = 0;
	m_motorAcceleration[index] = acceleration;
	desc.m_jointAccel[index] = acceleration;
}

inline void ndJointBilateralConstraint::SetLowerFriction(ndConstraintDescritor& desc, ndFloat32 friction)
{
	const ndInt32 index = desc.m_rowsCount - 1;
	dAssert(index >= 0);
	dAssert(index < ndInt32 (m_maxDof));
	desc.m_forceBounds[index].m_low = dClamp(friction, ndFloat32(D_MIN_BOUND), ndFloat32(-0.001f));
	dAssert(desc.m_forceBounds[index].m_normalIndex == D_INDEPENDENT_ROW);

	#ifdef _DEBUG
	ndInt32 i0 = 0;
	ndInt32 i1 = index - 1;
	while ((i0 <= i1) && (desc.m_forceBounds[i0].m_normalIndex == D_INDEPENDENT_ROW)) i0++;
	while ((i1 >= i0) && (desc.m_forceBounds[i1].m_normalIndex != D_INDEPENDENT_ROW)) i1--;
	dAssert((i0 - i1) == 1);
	if ((i0 - i1) != 1) 
	{
		dTrace(("make sure that friction joint are issue at last\n"));
	}
	#endif
}

inline void ndJointBilateralConstraint::SetHighFriction(ndConstraintDescritor& desc, ndFloat32 friction)
{
	const ndInt32 index = desc.m_rowsCount - 1;
	dAssert(index >= 0);
	dAssert(index < ndInt32(m_maxDof));
	
	desc.m_forceBounds[index].m_upper = dClamp(friction, ndFloat32(0.001f), ndFloat32(D_MAX_BOUND));
	dAssert(desc.m_forceBounds[index].m_normalIndex == D_INDEPENDENT_ROW);

	#ifdef _DEBUG
	ndInt32 i0 = 0;
	ndInt32 i1 = index - 1;
	while ((i0 <= i1) && (desc.m_forceBounds[i0].m_normalIndex == D_INDEPENDENT_ROW)) i0++;
	while ((i1 >= i0) && (desc.m_forceBounds[i1].m_normalIndex != D_INDEPENDENT_ROW)) i1--;
	dAssert((i0 - i1) == 1);
	if ((i0 - i1) != 1) 
	{
		dTrace(("make sure that friction joint are issue at last\n"));
	}
	#endif
}

inline void ndJointBilateralConstraint::JacobianDerivative(ndConstraintDescritor&)
{
	//dAssert(0);
	dTrace(("error: this joint is an interface\n"));
}

inline void ndJointBilateralConstraint::SetSkeletonFlag(bool flag)
{
	m_isInSkeleton = flag ? 1 : 0;
}

inline bool ndJointBilateralConstraint::IsCollidable() const
{
	return m_enableCollision ? true : false;
}

inline bool ndJointBilateralConstraint::IsBilateral() const
{
	return true;
}

inline void ndJointBilateralConstraint::SetCollidable(bool state)
{
	m_enableCollision = state ? 1 : 0;
}

inline ndVector ndJointBilateralConstraint::GetForceBody0() const
{
	return m_forceBody0;
}

inline ndVector ndJointBilateralConstraint::GetTorqueBody0() const
{
	return m_torqueBody0;
}

inline ndVector ndJointBilateralConstraint::GetForceBody1() const
{
	return m_forceBody1;
}

inline ndVector ndJointBilateralConstraint::GetTorqueBody1() const
{
	return m_torqueBody1;
}

inline ndFloat32 ndJointBilateralConstraint::GetDiagonalRegularizer(const ndConstraintDescritor& desc) const
{
	const ndInt32 index = desc.m_rowsCount - 1;
	dAssert(index >= 0);
	dAssert(index < ndInt32(m_maxDof));
	return desc.m_diagonalRegularizer[index];
}

inline void ndJointBilateralConstraint::SetDiagonalRegularizer(ndConstraintDescritor& desc, ndFloat32 stiffness)
{
	const ndInt32 index = desc.m_rowsCount - 1;
	dAssert(index >= 0);
	dAssert(index < ndInt32(m_maxDof));
	desc.m_diagonalRegularizer[index] = dClamp(stiffness, ndFloat32(0.0f), ndFloat32(1.0f));
}

inline bool ndJointBilateralConstraint::IsSkeleton() const
{
	const ndJointBilateralSolverModel mode = GetSolverModel();
	bool test = false;
	test = test || (mode == m_jointkinematicOpenLoop);
	test = test || (mode == m_jointkinematicCloseLoop);
	//test = test || (mode == m_jointkinematicHintOpenLoop);
	return test;
}

inline void ndJointBilateralConstraint::ReplaceSentinel(ndBodyKinematic* const sentinel)
{
	m_body1 = sentinel;
}

#endif

