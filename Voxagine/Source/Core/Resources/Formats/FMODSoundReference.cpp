#include "pch.h"
#include "FMODSoundReference.h"

#include "Core/Platform/Audio/AudioContext.h"

FMODSoundReference::~FMODSoundReference()
{
	Free();
}

bool FMODSoundReference::Load(const std::string& filePath)
{
	bool loaded = m_pAudioContext->CreateSound(filePath, Sound, filePath.find("_BGM") == std::string::npos);

	/*	try to load the frame config file */
	FH handle = m_pFileSystem->OpenFile((filePath + ".cfg").c_str(), FSOF_READ | FSOF_BINARY);

	if (handle)
	{
		FSize fileSize = m_pFileSystem->GetFileSize(handle);

		char* pBuffer = new char[fileSize];

		m_pFileSystem->Read(handle, pBuffer, 1, fileSize);
		m_pFileSystem->CloseFile(handle);

		uint32_t index = 0;

		std::string path = "";

		for (uint32_t i = 0; i < fileSize; ++i)
		{
			char c = pBuffer[i];

			if (c == ';')
			{
				unsigned long ul = std::stoul(path, nullptr, 0);

				if (index == 0)
					m_uiLoopStart = ul;
				else
					m_uiLoopEnd = ul;

				index++;

				if (index >= 2)
					break;

				path.clear();

				continue;
			}

			path += c;
		}
	}

	m_bIsLoaded = loaded;
	return loaded;
}

void FMODSoundReference::Free()
{
	if (Sound)
		((FMOD::Sound*)Sound)->release();
}
