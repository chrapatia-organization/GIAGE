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


#ifndef _D_FILE_BROWSER_H_
#define _D_FILE_BROWSER_H_

bool dGetOpenFileNamePLY(char* const fileName, int maxSize);

bool dGetLoadNdFileName(char* const fileName, int maxSize);
bool dGetSaveNdFileName(char* const fileName, int maxSize);

#endif 

