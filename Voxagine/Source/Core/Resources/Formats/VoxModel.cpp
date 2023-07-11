#include "pch.h"
#include "VoxModel.h"

#include "Core/ECS/Systems/Physics/VoxelGrid.h"
#include "Core/Platform/Rendering/Objects/Mapper.h"

#include "Core/Platform/Rendering/RenderContextInc.h"

#include "VoxFile.h"
#include "Core/Platform/Rendering/RenderContext.h"
#include <Core/Platform/Platform.h>
#include <Core/Application.h>

VoxModel::~VoxModel()
{
	Free();
}

bool VoxModel::Load(const std::string& filePath)
{
	/* Free old data */
	Free();

	/* Open file */
	FH fileHandle = m_pFileSystem->OpenFile(filePath.c_str(), FSOF_READ | FSOF_BINARY);
	if (fileHandle == INVALID_FH)
	{
		printf("Failed to vox file");
		return false;
	}

	/* File is loaded, read data */
	bool readResult = Read(fileHandle);

	m_pFileSystem->CloseFile(fileHandle);

	/* Failed to load file, free memory */
	if (!readResult)
		Free();

	/*	try to load the frame config file */
	FH handle = m_pFileSystem->OpenFile((filePath + ".cfg").c_str(), FSOF_READ | FSOF_BINARY);

	if (handle && !m_Frames.empty())
	{
		m_bHasAudio = true;

		FSize fileSize = m_pFileSystem->GetFileSize(handle);

		char* pBuffer = new char[fileSize];

		m_pFileSystem->Read(handle, pBuffer, 1, fileSize);
		m_pFileSystem->CloseFile(handle);

		uint32_t index = 0;
		std::string path = "";

		auto& resourceManager = m_pContext->GetPlatform()->GetApplication()->GetResourceManager();

		for (uint32_t i = 0; i < fileSize; ++i)
		{
			char c = pBuffer[i];

			if (c == ';')
			{
				SoundReference* pReference = path.empty() ? nullptr : resourceManager.LoadSound(path);
				m_Frames[index].m_pAudioAsset = pReference ? pReference->GetRefPath().empty() ? nullptr : pReference : nullptr;

				index++;

				if (index >= m_Frames.size())
					break;

				path.clear();

				continue;
			}

			path += c;
		}
	}

	m_bIsLoaded = true;
	return true;
}

void VoxModel::Free()
{
	for (VoxFrame& frame : m_Frames) {
		delete[] frame.m_pData;
		frame.m_pData = nullptr;

		delete[] frame.m_pColors;
		delete[] frame.m_pPositions;

		frame.m_uiSolidVoxelCount = 0;

		m_pContext->GetModelManager()->DestroyModel(frame.m_uiMapperID);

		if (frame.m_pAudioAsset)
			frame.m_pAudioAsset->Release();
	}

	m_bIsCustomPalette = false;
	m_iVersion = 0;
}

void VoxModel::SaveBatch(FileSystem* pFileSystem, const std::string & path, const std::vector<VoxFrame>& frames, const RGBA(&pallette)[256])
{
	VoxFileHelper helper(pFileSystem, path);
	
	{
		size_t voxCount = 0;
		for (auto& frame : frames)
		{
			voxCount += frame.m_uiVoxelCount;
		}

		auto amountOfFrames = frames.size();
		helper << VoxHeader(sizeof(VoxPack) + amountOfFrames * (sizeof(VoxSize) + sizeof(VoxXYZI)) + voxCount * sizeof(VoxVoxel) + sizeof(VoxRGBA) );
		helper << VoxPack( amountOfFrames );
	}

	for (auto& frame : frames)
	{
		auto size = frame.GetSize();
		helper << VoxSize{ (int)size.x, (int)size.z, (int)size.y }; /* Swap axes */

		helper << VoxXYZI{ (int)frame.m_uiVoxelCount };

		for ( auto i = 0; i < frame.m_uiVoxelCount; i++ )
		{
			auto& data = frame.m_pData[i];
			helper << VoxVoxel(data.x, data.y, data.z, data.colorIndex);
		}
	
	}

	helper << VoxRGBA(pallette);
}

void VoxModel::Save(FileSystem* pFileSystem, const std::string& path, UVector3 size, const uint32_t* data)
{
	VoxFileHelper helper(pFileSystem, path);

	int voxCount = size.x * size.y * size.z;

	helper << VoxHeader(sizeof(VoxPack) + sizeof(VoxSize) + sizeof(VoxXYZI) + voxCount * sizeof(VoxVoxel));

	helper << VoxPack{ 1 };
	
	helper << VoxSize{ (int)size.x, (int)size.z, (int)size.y}; /* Swap axes */

	helper << VoxXYZI{ voxCount };

	for (uint32_t x = 0; x < size.x; ++x)
	{
		for (uint32_t y = 0; y < size.y; ++y)
		{
			for (uint32_t z = 0; z < size.z; ++z)
			{

				/* Swap axes */
				uint32_t xNew = x;
				uint32_t yNew = z;
				uint32_t zNew = y;	

				helper << VoxVoxel(xNew, yNew, zNew, 5 );

			}

		}

	}

	RGBA pallette[256];
	memset(pallette, 5, 256);

	helper << VoxRGBA(pallette);


}

void VoxModel::Save2(FileSystem* pFileSystem, const std::string& path, UVector3 size, const uint32_t* data)
{
#define UINT_TO_CHAR(x) { ((char*)&x)[0], ((char*)&x)[1], ((char*)&x)[2], ((char*)&x)[3] }
#define COL_TO_CHAR(x) { static_cast<const char>((x >> 16) & 0xFF), static_cast<const char>((x >> 8) & 0xFF), static_cast<const char>(x & 0xFF), static_cast<const char>((x >> 24) & 0xFF) }
#define COPY_MEM(x, y) std::memcpy(&output[memoryOffset], x, y); memoryOffset += y;
#define SET_MEM(x, y) std::memset(&output[memoryOffset], x, y); memoryOffset += y;

	uint32_t memoryOffset = 0;

	std::unordered_set<uint32_t> pal;
	std::vector<uint32_t> palette;
	std::vector<VoxFrame::Data> colors;

	uint32_t voxCount = size.x * size.y * size.z;

	pal.reserve(256);
	palette.reserve(256);
	colors.resize(voxCount);

	// std::fill(palette.begin(), palette.end(), UINT32_MAX);

	for (uint32_t x = 0; x < size.x; ++x)
	{
		for (uint32_t y = 0; y < size.y; ++y)
		{
			for (uint32_t z = 0; z < size.z; ++z)
			{
				uint32_t id = (uint32_t)(x + y * size.x + (z * size.x * size.y));
				uint32_t id2 = (uint32_t)(x + z * size.x + (y * size.x * size.z));

				/* Swap axes */
				uint32_t xNew = x;
				uint32_t yNew = z;
				uint32_t zNew = y;

				//xNew = size.x - 1 - x;
				//yNew = size.y - 1 - y;

				// Check for existing palette color
				uint32_t index = 0;

				if (palette.size() < 255)
				{
					std::pair<std::unordered_set<uint32_t>::iterator, bool> find = pal.insert(data[id]);

					if (!find.second)
					{
						index = static_cast<uint32_t>(std::distance(pal.begin(), find.first));
					}
					else
					{
						uint32_t paletteData;
						const char PaletteColor[4] = COL_TO_CHAR(data[id]);
						std::memcpy(&paletteData, PaletteColor, 4);

						palette.push_back(paletteData);
						index = static_cast<uint32_t>(palette.size() - 1);
					}
				}

				VoxFrame::Data colData;
				colData.x = static_cast<unsigned char>(xNew);
				colData.y = static_cast<unsigned char>(yNew);
				colData.z = static_cast<unsigned char>(zNew);
				colData.colorIndex = static_cast<unsigned char>(index);
				colors[id2] = colData;
			}
		}
	}

	static const uint32_t MV_UI_VERSION = 150;
	static const char MV_VERSION[4] = UINT_TO_CHAR(MV_UI_VERSION);

	static const char* ID_VOX = "VOX ";
	static const char* ID_MAIN = "MAIN";
	static const char* ID_SIZE = "SIZE";
	static const char* ID_XYZI = "XYZI";
	static const char* ID_RGBA = "RGBA";
	static const char* ID_PACK = "PACK";

	static const uint32_t uintSize = sizeof(uint32_t);

	uint32_t paletteCount = static_cast<uint32_t>(palette.size());
	static const uint32_t MAINContentSize = 12;

	uint32_t ColorDataSize = voxCount * uintSize;
	const uint32_t PaletteDataSize = paletteCount * uintSize;

	const uint32_t XYZIContentSize = 4 + ColorDataSize;
	uint32_t totalChunks = (MAINContentSize + 3 * uintSize) + (MAINContentSize + XYZIContentSize) + (MAINContentSize + PaletteDataSize);
	uint32_t dataSize = 16 + totalChunks;
	char* output = new char[dataSize];

	COPY_MEM(ID_VOX, 4);
	COPY_MEM(MV_VERSION, 4);

	// MAIN chunk
	COPY_MEM(ID_MAIN, 4);
	SET_MEM(NULL, 4);

	const char MAINSize[4] = UINT_TO_CHAR(totalChunks);

	COPY_MEM(MAINSize, 4);

	// SIZE chunk
	COPY_MEM(ID_SIZE, 4);

	const char MAINCSize[4] = UINT_TO_CHAR(MAINContentSize);
	COPY_MEM(MAINCSize, 4);
	SET_MEM(NULL, 4);

	const char ModelX[4] = UINT_TO_CHAR(size.x);
	COPY_MEM(ModelX, 4);

	const char ModelY[4] = UINT_TO_CHAR(size.z);
	COPY_MEM(ModelY, 4);

	const char ModelZ[4] = UINT_TO_CHAR(size.y);
	COPY_MEM(ModelZ, 4);

	// VOXEL chunk
	COPY_MEM(ID_XYZI, 4);

	const char XYZISize[4] = UINT_TO_CHAR(XYZIContentSize);
	COPY_MEM(XYZISize, 4);
	SET_MEM(NULL, 4);

	const char VoxCount[4] = UINT_TO_CHAR(voxCount);
	COPY_MEM(VoxCount, 4);

	COPY_MEM(colors.data(), ColorDataSize);

	COPY_MEM(ID_RGBA, 4);

	const char PaletteCount[4] = UINT_TO_CHAR(PaletteDataSize);
	COPY_MEM(PaletteCount, 4);
	SET_MEM(NULL, 4);

	COPY_MEM(palette.data(), PaletteDataSize);

	// Last color black
	// SET_MEM(&output[76 + ColorSize], 0xff000000, 4);

	FH handle = pFileSystem->OpenFile(path.c_str(), FSOpenFlags::FSOF_WRITE);
	pFileSystem->Write(handle, (const void*)output, sizeof(char), dataSize);
	pFileSystem->CloseFile(handle);

	delete[] output;
}

VoxFrame VoxModel::CreateHollowFrame(const VoxFrame& frame)
{
	std::vector<VoxFrame::Data> newData;

	for (uint32_t i = 0; i < frame.m_uiVoxelCount; ++i)
	{
		const VoxFrame::Data& data = frame.m_pData[i];

		/* Swap axes */
		Vector3 voxelPos = { data.x, data.z, data.y };
		
		/* Restore offset */
		voxelPos -= frame.m_v3FitSizeOffset;

		/* Flip axes */
		voxelPos.x = frame.m_v3Size.x - 1.0f - voxelPos.x;
		voxelPos.z = frame.m_v3Size.z - 1.0f - voxelPos.z;
		
		bool hasEmptyNeighbours = false;

		for (int8_t z = -1; z <= 1; ++z)
		{
			for (int8_t y = -1; y <= 1; ++y)
			{
				for (int8_t x = -1; x <= 1; ++x)
				{
					if(x == 0 && y == 0 && z == 0 )
					{
						continue; 
					}

					Vector3 neighbourPos = { voxelPos.x + x, voxelPos.y + y, voxelPos.z + z };

					if (neighbourPos.x >= frame.m_v3Size.x || neighbourPos.x < 0.f ||
						neighbourPos.y >= frame.m_v3Size.y || neighbourPos.y < 0.f ||
						neighbourPos.z >= frame.m_v3Size.z || neighbourPos.z < 0.f)
					{
						hasEmptyNeighbours = true;
						goto HasEmptyNeighbour;
					}

					hasEmptyNeighbours = hasEmptyNeighbours || frame.GetVoxelColor(
						static_cast<uint8_t>(neighbourPos.x),
						static_cast<uint8_t>(neighbourPos.y),
						static_cast<uint8_t>(neighbourPos.z)
					) == 0;
				}
			}
		}
		
		HasEmptyNeighbour:

		if (hasEmptyNeighbours)
		{
			newData.push_back(data);
		}
		else
		{
			//newData.push_back(data);
		}

	}

	VoxFrame res = frame;
	res.m_uiVoxelCount = newData.size();
	res.m_pData = new VoxFrame::Data[res.m_uiVoxelCount];
	std::memcpy(res.m_pData, &newData[0], res.m_uiVoxelCount * sizeof(VoxFrame::Data));

	return std::move(res);
}

void VoxModel::MakeHollow(const std::string& filePath)
{
	Reset(); // Remove all modifications 

	for (auto& frame : m_Frames)
	{
		VoxFrame tmp = CreateHollowFrame(frame);
		delete[] frame.m_pData;
		frame = std::move(tmp);
	}
	
	Reset(); // Apply new changed in pData

	SaveBatch(m_pFileSystem, filePath, m_Frames, m_Palette);
}


const VoxFrame* VoxModel::GetFrame(size_t frame) const
{
	if (m_Frames.size() > frame)
		return &m_Frames[frame];

	return nullptr;
}

void VoxModel::Reset()
{
	for (VoxFrame& frame : m_Frames) {
		if (frame.GetMapperID() != UINT_MAX)
		{
			m_pContext->GetModelManager()->DestroyModel(frame.GetMapperID());
			frame.m_uiMapperID = UINT_MAX;
		}

		delete[] frame.m_pColors;
		delete[] frame.m_pPositions;

		frame.m_uiSolidVoxelCount = 0;

		if (!frame.m_pData)
			return;

		/* Fit model size */
		unsigned char maxX = 0;
		unsigned char maxY = 0;
		unsigned char maxZ = 0;

		unsigned char minX = UCHAR_MAX;
		unsigned char minY = UCHAR_MAX;
		unsigned char minZ = UCHAR_MAX;

		uint32_t uiColor = 0;

		for (uint32_t i = 0; i < frame.m_uiVoxelCount; ++i) {
			if (m_bIsCustomPalette)
				uiColor = *((uint32_t*)&m_Palette[frame.m_pData[i].colorIndex]);
			else
				uiColor = m_uiDefaultPalette[frame.m_pData[i].colorIndex];

			if (VColor(uiColor).inst.Colors.a == 0)
				continue;

			unsigned char x = frame.m_pData[i].x;
			unsigned char y = frame.m_pData[i].y;
			unsigned char z = frame.m_pData[i].z;

			minX = std::min(minX, x);
			minY = std::min(minY, z);
			minZ = std::min(minZ, y);

			maxX = std::max(maxX, x);
			maxY = std::max(maxY, z);
			maxZ = std::max(maxZ, y);
		}

		Vector3 v3Original = frame.m_v3Size;

		frame.m_v3Size.x = std::min(frame.m_v3Size.x, (float)(maxX - minX) + 1.0f);
		frame.m_v3Size.y = std::min(frame.m_v3Size.y, (float)(maxY - minY) + 1.0f);
		frame.m_v3Size.z = std::min(frame.m_v3Size.z, (float)(maxZ - minZ) + 1.0f);

		frame.m_v3FitSizeOffset = Vector3(
			(float)minX,
			(float)minY,
			(float)minZ
		);

		/* Create voxel data */
		uint32_t volumeVoxelCount = static_cast<uint32_t>(frame.m_v3Size.x * frame.m_v3Size.y * frame.m_v3Size.z);

		frame.m_pColors = new uint32_t[volumeVoxelCount];
		frame.m_pPositions = new uint32_t[volumeVoxelCount];

		for (uint32_t i = 0; i < frame.m_uiVoxelCount; ++i)
		{
			VoxFrame::Data& data = frame.m_pData[i];

			/* Store color bytes to four-byte integer */
			if (m_bIsCustomPalette)
				uiColor = *((uint32_t*)&m_Palette[data.colorIndex]);
			else
				uiColor = m_uiDefaultPalette[data.colorIndex];

			if (VColor(uiColor).inst.Colors.a == 0)
				continue;

			/* Swap axes */
			float x = data.x - (float)minX;
			float y = data.z - (float)minY;
			float z = data.y - (float)minZ;

			/* Flip axes */
			x = frame.m_v3Size.x - 1.0f - x;
			z = frame.m_v3Size.z - 1.0f - z;

			frame.m_pColors[frame.m_uiSolidVoxelCount] = uiColor;
			frame.m_pPositions[frame.m_uiSolidVoxelCount] = VColor(
				static_cast<unsigned char>(x),
				static_cast<unsigned char>(y),
				static_cast<unsigned char>(z),
				static_cast<unsigned char>(0)
			).inst.Color;

			frame.m_uiSolidVoxelCount++;
		}

		// Map model data to GPU
		/*Mapper::Info mapperInfo;
		mapperInfo.m_uiElementCount = frame.m_uiSolidVoxelCount * 2;
		mapperInfo.m_uiElementSize = sizeof(uint32_t);
		mapperInfo.m_GPUAccessType = E_READ_ONLY;

		Mapper* pMapper = new Mapper(m_pContext, mapperInfo);
		pMapper->Map();
		std::memcpy(&pMapper->GetData()[0], frame.m_pPositions, frame.m_uiSolidVoxelCount * sizeof(uint32_t));
		std::memcpy(&pMapper->GetData()[frame.m_uiSolidVoxelCount], frame.m_pColors, frame.m_uiSolidVoxelCount * sizeof(uint32_t));
		pMapper->Unmap();*/

		// delete[] frame.m_pColors;
		// delete[] frame.m_pPositions;

		//frame.m_uiMapperID = m_pContext->GetModelManager()->AddModel(pMapper);
	}
}

bool VoxModel::Read(FH fileHandle)
{
	const int MV_VERSION = 150;

	const int ID_VOX = GetID('V', 'O', 'X', ' ');
	const int ID_MAIN = GetID('M', 'A', 'I', 'N');
	const int ID_SIZE = GetID('S', 'I', 'Z', 'E');
	const int ID_XYZI = GetID('X', 'Y', 'Z', 'I');
	const int ID_RGBA = GetID('R', 'G', 'B', 'A');
	const int ID_PACK = GetID('P', 'A', 'C', 'K');

	/* Magic number */
	const int iMagic = ReadInt(fileHandle);

	if (iMagic != ID_VOX) {
		Error("The magic number does not match");
		return false;
	}

	/* Version */
	m_iVersion = ReadInt(fileHandle);

	if (m_iVersion != MV_VERSION) {
		Error("The version does not match");
		return false;
	}

	/* Main chunk */
	Chunk MainChunk;
	ReadChunk(fileHandle, MainChunk);

	if (MainChunk.id != ID_MAIN) {
		Error("The main chunk could not found");
		return false;
	}

	/* Skip content of main chunk */
	m_pFileSystem->FileSeek(fileHandle, MainChunk.contentSize, FSSO_CUR);

	/* Read child chunks */
	while (m_pFileSystem->FileTell(fileHandle) < MainChunk.end) {
		/* Read chunk header */
		Chunk Sub;
		ReadChunk(fileHandle, Sub);

		if (Sub.id == ID_SIZE) {
			/* Size */
			VoxFrame frame;
			frame.m_pModel = this;
			
			frame.m_v3Size.x = (float)ReadInt(fileHandle);
			frame.m_v3Size.z = (float)ReadInt(fileHandle);
			frame.m_v3Size.y = (float)ReadInt(fileHandle);

			m_Frames.push_back(frame);
		}
		else if (Sub.id == ID_XYZI) {
			VoxFrame& frame = m_Frames.back();

			/* Voxel count */
			frame.m_uiVoxelCount = ReadInt(fileHandle);

			if (frame.m_uiVoxelCount < 0) {
				Error("negative number of voxels");
				return false;
			}

			/* Voxel data */
			if (frame.m_uiVoxelCount > 0) {
				frame.m_pData = new VoxFrame::Data[static_cast<size_t>(frame.m_uiVoxelCount)];
				m_pFileSystem->Read(fileHandle, frame.m_pData, sizeof(VoxFrame::Data), frame.m_uiVoxelCount);
			}
		}
		else if (Sub.id == ID_RGBA) {
			/* Last color is unused, so we only need to read 255 colors */
			m_bIsCustomPalette = true;
			m_pFileSystem->Read(fileHandle, m_Palette + 1, sizeof(RGBA), 255);

			/* NOTICE: skip the last reserved color */
			RGBA Reserved;
			m_pFileSystem->Read(fileHandle, &Reserved, sizeof(RGBA), 1);
		}
		//else if (Sub.id == ID_PACK)
		//{
		//	int chunks = ReadInt(fileHandle);
		//	bool yelll = false;
		//}

		/* Skip unread bytes of current chunk or the whole unused chunk */
		m_pFileSystem->FileSeek(fileHandle, Sub.end, FSSO_SET);
	}

	/* Store adjustable voxel data with correct colors */
	Reset();
	return true;
}

int VoxModel::GetID(int iA, int iB, int iC, int iD)
{
	return (iA) | (iB << 8) | (iC << 16) | (iD << 24);
}

void VoxModel::ReadChunk(FH fileHandle, Chunk& Chunk)
{
	/* Read chunk */
	Chunk.id = ReadInt(fileHandle);
	Chunk.contentSize = ReadInt(fileHandle);
	Chunk.childrenSize = ReadInt(fileHandle);

	/* End of chunk : used for skipping the whole chunk */
	FSize size = m_pFileSystem->FileTell(fileHandle);
	Chunk.end = size + Chunk.contentSize + Chunk.childrenSize;
}

int VoxModel::ReadInt(FH fileHandle)
{
	int v = 0;
	m_pFileSystem->Read(fileHandle, &v, 4, 1);
	return v;
}

void VoxModel::Error(const char* cInfo) const
{
	printf("[Error] MV_VoxelModel :: %s\n", cInfo);
}

VoxFileHelper::VoxFileHelper(FileSystem* pFileSystem, const std::string & path) :
	m_bufferSize(0),
	m_pFileSystem(pFileSystem)
{
	m_buffer = (uint8_t*)malloc(s_maxBufferSize);
	m_ptr = m_buffer;
	m_handle = m_pFileSystem->OpenFile(path.c_str(), FSOpenFlags::FSOF_WRITE | FSOpenFlags::FSOF_BINARY);
}

VoxFileHelper::~VoxFileHelper()
{
	if (m_bufferSize > 0)
	{
		m_pFileSystem->Write(m_handle, m_buffer, sizeof(uint8_t), m_bufferSize);
	}

	m_pFileSystem->CloseFile(m_handle);
	free(m_buffer);
}

uint32_t VoxFrame::GetVoxelColor(uint8_t x, uint8_t y, uint8_t z) const
{
	assert(x >= 0 && x < m_v3Size.x);
	assert(y >= 0 && y < m_v3Size.y);
	assert(z >= 0 && z < m_v3Size.z);

	uint32_t uiPosition = static_cast<uint32_t>(x + y * m_v3Size.x + (z * m_v3Size.x * m_v3Size.y));

	for (uint32_t i = 0; i < m_uiSolidVoxelCount; ++i)
	{
		if (m_pPositions[i] == uiPosition)
			return m_pColors[i];
	}
}