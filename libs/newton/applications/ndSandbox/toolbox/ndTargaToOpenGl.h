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



#ifndef __TARGA_TO_OPENGL__ 
#define __TARGA_TO_OPENGL__ 

#include "ndSandboxStdafx.h"


enum TextureImageFormat
{
	m_rgb,
	m_rgba,
	m_luminace,
};

GLuint GetDefaultTexture();
GLuint LoadTexture(const char* const filename);

GLuint AddTextureRef (GLuint texture);
void ReleaseTexture (GLuint texture);

const char* FindTextureById (GLuint textureID);

#endif


