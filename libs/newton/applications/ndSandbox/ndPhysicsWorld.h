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
#ifndef __PHYSICS_WORLD_H__
#define __PHYSICS_WORLD_H__

#include "ndSandboxStdafx.h"

#define MAX_PHYSICS_FPS			60.0f

class ndSoundManager;
class ndDemoEntityManager;

class ndPhysicsWorld: public ndWorld
{
	public:
	ndPhysicsWorld(ndDemoEntityManager* const manager);
	virtual ~ndPhysicsWorld();

	void AdvanceTime(ndFloat32 timestep);
	ndDemoEntityManager* GetManager() const;
	ndSoundManager* GetSoundManager() const;

	void QueueBodyForDelete(ndBody* const body);

	bool LoadScene(const char* const path);
	void SaveScene(const char* const path);
	void SaveSceneModel(const char* const path);

	private:
	void DeletePendingObjects();
	void OnPostUpdate(ndFloat32 timestep);

	ndDemoEntityManager* m_manager;
	ndSoundManager* m_soundManager;
	ndFloat32 m_timeAccumulator;
	ndArray<ndBody*> m_deletedBodies;
	ndAtomic<bool> m_hasPendingObjectToDelete;
	ndSpinLock m_deletedLock;
};

#endif