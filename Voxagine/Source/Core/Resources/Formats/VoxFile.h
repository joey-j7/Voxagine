#pragma once

#include "Core/Resources/ReferenceObject.h"

#include <stdint.h>
#include <string>

struct VoxVoxel
{
	VoxVoxel() = default;
	VoxVoxel(const uint8_t x, const uint8_t y, const uint8_t z, const uint8_t i) :
		m_x(x), m_y(y), m_z(z), m_i(i)
	{}

	uint8_t m_x, m_y, m_z, m_i;
};

struct VoxChunk
{
	VoxChunk() = delete;

	VoxChunk(const char* name = "NULL") :
		m_numOfBytes(0),
		m_numOfBytesChildren(0)
	{
		std::memcpy(m_name, name, 4);
	};

	union
	{
		char m_name[4];
		int m_magicNumber;
	};

	bool operator==(const VoxChunk& rhs) { return m_magicNumber == rhs.m_magicNumber; };

	int m_numOfBytes;
	int m_numOfBytesChildren;
};

struct VoxHeader
{
	VoxHeader() = default;
	VoxHeader(const int m_numOfBytesChildren) :
		m_version(150)
	{
		m_main.m_numOfBytesChildren = m_numOfBytesChildren;
		std::memcpy(m_name, "VOX ", 4);
	};

	char m_name[4];
	int m_version;
	VoxChunk m_main = { "MAIN" };
};

struct VoxPack
{
	VoxPack() = delete;

	VoxPack(const int numOfModels) :
		m_numOfModels(numOfModels)
	{
		m_pack.m_numOfBytes = sizeof(int);
	}

	VoxChunk m_pack = { "PACK" };
	int m_numOfModels;
};

struct VoxSize
{
	VoxSize() = delete;

	VoxSize(const int x, const int y, const int z) :
		m_xDim(x),
		m_yDim(y),
		m_zDim(z)
	{
		m_size.m_numOfBytes = 3 * sizeof(int);
	}

	VoxChunk m_size = { "SIZE" };
	int m_xDim = 0, m_yDim = 0, m_zDim = 0;
};

struct VoxXYZI
{
	VoxXYZI() = delete;
	VoxXYZI(const int numVoxels) :
		m_numVoxels(numVoxels)
	{
		m_xyzi.m_numOfBytes = sizeof(int) + numVoxels * sizeof(VoxVoxel);
	}

	VoxChunk m_xyzi = { "XYZI" };
	int m_numVoxels = 0;
};


struct VoxRGBA
{
	VoxRGBA() = delete;
	VoxRGBA(const VoxModel::RGBA(&pallette)[256])
	{
		m_rgba.m_numOfBytes = sizeof(pallette);
		// The reader shifts the pallette by 4 bytes making pallette[0] == 0, as a sort of "unused" voxel
		// to compensate for that I start reading from pallette[1] and add the last empty voxel back at the end
		std::memcpy(m_pallette, &pallette[1], m_rgba.m_numOfBytes - sizeof(VoxModel::RGBA));
		m_pallette[255] = {};
	}

	VoxChunk m_rgba = { "RGBA" };
	VoxModel::RGBA m_pallette[256];
};



class VoxFileHelper
{
public:
	VoxFileHelper() = delete;
	VoxFileHelper(FileSystem* pFileSystem, const std::string& path);
	~VoxFileHelper();

	template<typename T>
	void operator<<(const T& data);

private:
	uint8_t* m_buffer;
	uint8_t* m_ptr;
	size_t m_bufferSize;
	FileSystem* m_pFileSystem;
	FH m_handle;

	static const size_t s_maxBufferSize = sizeof(VoxRGBA);

};

template<typename T>
void VoxFileHelper::operator<<(const T& data)
{
	static_assert(s_maxBufferSize >= sizeof(T), "Max buffer size needs to be bigger!");

	if (m_bufferSize + sizeof(T) > s_maxBufferSize)
	{
		m_pFileSystem->Write(m_handle, m_buffer, sizeof(uint8_t), m_bufferSize);
		m_bufferSize = 0;
		m_ptr = m_buffer;
	}

	*reinterpret_cast<T*>(m_ptr) = data;
	m_ptr += sizeof(T);
	m_bufferSize += sizeof(T);

}