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
#include "ndTargaToOpenGl.h"

class ndTextureEntry: public ndRefCounter<ndTextureEntry>
{
	public:
	ndTextureEntry()
	{
	}

	GLuint m_textureID;
	ndString m_textureName;
};

class ndTextureCache: public ndTree<ndTextureEntry, ndUnsigned64>
{
	public: 
	GLuint GetTexture(const char* const texName)
	{
		GLuint texID = 0;
		dAssert (texName);

		//ndTextureEntry entry;
		//entry.m_textureName = texName;
		//entry.m_textureName.ToLower();
		//ndUnsigned64 crc = dCRC64 (entry.m_textureName.GetStr());
		char name[256];
		strcpy(name, texName);
		_strlwr(name);
		ndUnsigned64 crc = dCRC64(name);

		ndNode* node = Find(crc);
		if (node) 
		{
			node->GetInfo().AddRef();
			texID = node->GetInfo().m_textureID;
		}
		return texID;
	}

	void InsertText (const char* const texName, GLuint id) 
	{
		ndTextureEntry entry;
		entry.m_textureID = id;
		entry.m_textureName = texName;
		entry.m_textureName.ToLower();
		ndUnsigned64 crc = dCRC64 (entry.m_textureName.GetStr());
		Insert(entry, crc);
	}

	~ndTextureCache ()
	{
		Iterator iter (*this);
		for (iter.Begin(); iter; iter ++) 
		{
			glDeleteTextures(1, &iter.GetNode()->GetInfo().m_textureID);
		}
	}

	void RemoveById (GLuint id)
	{
		Iterator iter (*this);
		for (iter.Begin(); iter; iter ++) 
		{
			ndTextureEntry& entry = iter.GetNode()->GetInfo();
			if (entry.m_textureID == id) 
			{
				if (entry.GetRef() == 1) 
				{
					glDeleteTextures(1, &id);
					Remove (iter.GetNode());
				}
				break;
			}
		}
	}

	ndNode* FindById (GLuint id) const
	{
		Iterator iter (*this);
		for (iter.Begin(); iter; iter ++) 
		{
			if (iter.GetNode()->GetInfo().m_textureID == id) 
			{
				//return iter.GetNode()->GetInfo().m_textureName.GetStr();
				return iter.GetNode();
			}
		}
		return nullptr;
	}

	static ndTextureCache& GetChache()
	{
		static ndTextureCache texCache;
		return texCache;
	}
};


static GLuint LoadTargaImage(const char* const cacheName, const char* const buffer, ndInt32 width, ndInt32 hight, TextureImageFormat format)
{
	// Get width, height, and depth of texture
	GLint iWidth = width;
	GLint iHeight = hight;

	GLenum eFormat = GL_RGBA;
	GLint iComponents = 4;
	switch (format)
	{
	case m_rgb:
		// Most likely case
		eFormat = GL_BGR;
		//eFormat = GL_RGB;
		iComponents = 4;
		break;

	case m_rgba:
		eFormat = GL_BGRA;
		//eFormat = GL_RGBA;
		iComponents = 4;
		break;

	case m_luminace:
		//eFormat = GL_LUMINANCE;
		eFormat = GL_LUMINANCE_ALPHA;
		//eFormat = GL_ALPHA;
		iComponents = 4;
		break;
	};

	GLuint texture = 0;
	glGenTextures(1, &texture);
	if (texture)
	{
		glBindTexture(GL_TEXTURE_2D, texture);

		// select modulate to mix texture with color for shading
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		// when texture area is small, tri linear filter mip mapped
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

		// when texture area is large, bilinear filter the first mipmap
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		float anisotropic = 0.0f;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &anisotropic);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropic);

		// build our texture mip maps
		gluBuild2DMipmaps(GL_TEXTURE_2D, iComponents, iWidth, iHeight, eFormat, GL_UNSIGNED_BYTE, buffer);
		dAssert(glGetError() == GL_NO_ERROR);

		// Done with File
		ndTextureCache& cache = ndTextureCache::GetChache();
		cache.InsertText(cacheName, texture);
	}

	return texture;
}

//	Loads the texture from the specified file and stores it in iTexture. Note
//	that we're using the GLAUX library here, which is generally discouraged,
//	but in this case spares us having to write a bitmap loading routine.
GLuint LoadTexture(const char* const filename)
{
	#pragma pack(1)
	struct TGAHEADER
	{
		char identsize;					// Size of ID field that follows header (0)
		char colorMapType;				// 0 = None, 1 = palette
		char imageType;					// 0 = none, 1 = indexed, 2 = rgb, 3 = grey, +8=rle
		unsigned short colorMapStart;	// First color map entry
		unsigned short colorMapLength;	// Number of colors
		unsigned char colorMapBits;		// bits per palette entry
		unsigned short xstart;			// image x origin
		unsigned short ystart;			// image y origin
		unsigned short width;			// width in pixels
		unsigned short height;			// height in pixels
		char bits;						// bits per pixel (8 16, 24, 32)
		char descriptor;				// image descriptor
	};
	#pragma pack(8)

	char fullPathName[2048];
	dGetWorkingFileName (filename, fullPathName);
	ndTextureCache& cache = ndTextureCache::GetChache();
	GLuint texture = cache.GetTexture(fullPathName);
	if (!texture) 
	{
		FILE* const pFile = fopen (fullPathName, "rb");
		if(pFile == nullptr) 
		{
			return 0;
		}

		//dAssert (sizeof (TGAHEADER) == 18);
		// Read in header (binary) sizeof(TGAHEADER) = 18
		TGAHEADER tgaHeader;		// TGA file header
		size_t ret = fread(&tgaHeader, 18, 1, pFile);
		ret = 0;

		// Do byte swap for big vs little endian
		tgaHeader.colorMapStart = SWAP_INT16(tgaHeader.colorMapStart);
		tgaHeader.colorMapLength = SWAP_INT16(tgaHeader.colorMapLength);
		tgaHeader.xstart = SWAP_INT16(tgaHeader.xstart);
		tgaHeader.ystart = SWAP_INT16(tgaHeader.ystart);
		tgaHeader.width = SWAP_INT16(tgaHeader.width);
		tgaHeader.height = SWAP_INT16(tgaHeader.height);

		// Get width, height, and depth of texture
		ndInt32 width = tgaHeader.width;
		ndInt32 height = tgaHeader.height;
		short sDepth = tgaHeader.bits / 8;
		dAssert ((sDepth == 3) || (sDepth == 4));

		// Put some validity checks here. Very simply, I only understand
		// or care about 8, 24, or 32 bit targa's.
		if(tgaHeader.bits != 8 && tgaHeader.bits != 24 && tgaHeader.bits != 32) 
		{
			fclose(pFile);
			return 0;
		}

		// Calculate size of image buffer
		unsigned lImageSize = width * height * sDepth;

		// Allocate memory and check for success
		char* const pBits = (char*)ndMemory::Malloc (width * height * sizeof (ndInt32));
		if(pBits == nullptr) 
		{
			fclose(pFile);
			return 0;
		}

		// Read in the bits
		// Check for read error. This should catch RLE or other 
		// weird formats that I don't want to recognize
		ndInt32 readret = ndInt32 (fread(pBits, lImageSize, 1, pFile));
		if(readret != 1)  
		{
			fclose(pFile);
			delete[] pBits;
			return 0; 
		}

		TextureImageFormat format = m_rgb;
		switch(sDepth)
		{
			case 1:
				format = m_luminace;
				break;

			case 3:     
				format = m_rgb;
				break;

			case 4:
				format = m_rgba;
				break;
		};

		texture = LoadTargaImage(fullPathName, pBits, tgaHeader.width, tgaHeader.height, format);

		// Done with File
		fclose(pFile);
		ndMemory::Free (pBits);
	}
	return texture;
} 

#ifndef GL_BGR
#define GL_BGR 0x80E0
#define GL_BGRA 0x80E1
#endif

void ReleaseTexture (GLuint texture)
{
	ndTextureCache::GetChache().RemoveById (texture);
}

const char* FindTextureById (GLuint textureID)
{
	ndTextureCache& cache = ndTextureCache::GetChache();	
	ndTextureCache::ndNode* const node = cache.FindById (textureID);
	if (node) 
	{
		return node->GetInfo().m_textureName.GetStr();
	}
	return nullptr;
}

GLuint AddTextureRef (GLuint texture)
{
	ndTextureCache& cache = ndTextureCache::GetChache();	
	ndTextureCache::ndNode* const node = cache.FindById (texture);
	if (node) 
	{
		node->GetInfo().AddRef();
	}
	return texture;
}

GLuint GetDefaultTexture()
{
	return LoadTexture("default.tga");
}
