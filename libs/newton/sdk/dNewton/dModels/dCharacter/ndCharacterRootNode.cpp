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

#include "ndCoreStdafx.h"
#include "ndNewtonStdafx.h"
#include "ndCharacter.h"
#include "ndBodyDynamic.h"
#include "ndCharacterRootNode.h"

D_CLASS_REFLECTION_IMPLEMENT_LOADER(ndCharacterRootNode)

ndCharacterRootNode::ndCharacterRootNode(ndCharacter* const owner, ndBodyDynamic* const body)
	:ndCharacterNode(nullptr)
	,m_coronalFrame(dGetIdentityMatrix())
	,m_owner(owner)
	,m_body(body)
{
	SetCoronalFrame(m_body->GetMatrix());
}

ndCharacterRootNode::ndCharacterRootNode(const ndCharacterLoadDescriptor& desc)
	:ndCharacterNode(desc)
	,m_coronalFrame(dGetIdentityMatrix())
	,m_owner(nullptr)
	,m_body(nullptr)
{
	const nd::TiXmlNode* const xmlNode = desc.m_rootNode;

	const char* const name = xmlGetString(xmlNode, "name");
	SetName(name);
	m_localPose = xmlGetMatrix(xmlNode, "localPose");

	ndInt32 bodyHash = xmlGetInt(xmlNode, "bodyHash");
	m_body = (ndBodyDynamic*)desc.m_bodyMap->Find(bodyHash)->GetInfo();
	m_coronalFrame = xmlGetMatrix(xmlNode, "coronalFrame");
}

ndCharacterRootNode::~ndCharacterRootNode()
{
	delete m_body;
}

void ndCharacterRootNode::Save(const ndCharacterSaveDescriptor& desc) const
{
	nd::TiXmlElement* const childNode = new nd::TiXmlElement(ClassName());
	desc.m_rootNode->LinkEndChild(childNode);
	childNode->SetAttribute("hashId", desc.m_limbMap->GetCount());
	ndCharacterNode::Save(ndCharacterSaveDescriptor(desc, childNode));
	
	ndTree<ndInt32, const ndBodyKinematic*>::ndNode* bodyNode = desc.m_bodyMap->Find(m_body);
	if (!bodyNode)
	{
		bodyNode = desc.m_bodyMap->Insert(desc.m_bodyMap->GetCount(), m_body);
	}
	dAssert(bodyNode);

	xmlSaveParam(childNode, "name", GetName().GetStr());
	xmlSaveParam(childNode, "localPose", m_localPose);
	xmlSaveParam(childNode, "bodyHash", bodyNode->GetInfo());
	xmlSaveParam(childNode, "coronalFrame", m_coronalFrame);
}

//void ndCharacterRootNode::SetCoronalFrame(const dMatrix& frameInGlobalSpace)
//{
//	dMatrix matrix(m_body->GetMatrix());
//	m_coronalFrame = frameInGlobalSpace * matrix.Inverse();
//	m_coronalFrame.m_posit = dVector::m_wOne;
//}

//void ndCharacterRootNode::UpdateGlobalPose(ndWorld* const, dFloat32)
//{
//	// for now just; 
//}
