#include "pch.h"
#include "FMODContext.h"

#include "External/FMOD/fmod.hpp"

#if defined(EDITOR) || defined(DEBUG)
#include "External/FMOD/fmod_errors.h"
#include "Core/LoggingSystem/LogLevel.h"
#endif

#include "../Platform.h"
#include <Core/Application.h>
#include "Core/Resources/Formats/SoundReference.h"
#include "Core/System/FileSystem.h"

FileSystem* FMODContext::m_pFileSystem = nullptr;

#ifdef _ORBIS
static FMOD_RESULT ConvertResult(FSResult result)
{
	switch (result)
	{
	case FSR_OK:
		return FMOD_OK;
	case FSR_FAILED:
		return FMOD_ERR_FILE_BAD;
	case FSR_INVALID_HANDLE:
		return FMOD_ERR_INVALID_HANDLE;
	case FSR_ERR_EOF:
		return FMOD_ERR_FILE_EOF;
	default:
		return FMOD_ERR_FILE_BAD;
	}
}

static FMOD_RESULT FmodOpenFile(const char *name, unsigned int *filesize, void **handle, void *userdata)
{
	void* pFileHandle = new FH(FMODContext::m_pFileSystem->OpenFile(name, FSOF_READ | FSOF_BINARY));
	if ((*(FH*)pFileHandle) == INVALID_FH)
	{
		delete (FH*)pFileHandle;
		return FMOD_RESULT::FMOD_ERR_FILE_BAD;
	}

	*filesize = static_cast<unsigned int>(FMODContext::m_pFileSystem->GetFileSize((*(FH*)pFileHandle)));

	*handle = pFileHandle;
	return FMOD_RESULT::FMOD_OK;
}

static FMOD_RESULT FmodCloseFile(void *handle, void *userdata)
{
	FSResult result = FMODContext::m_pFileSystem->CloseFile(*(FH*)handle);
	if (result == FSR_OK)
	{
		delete (FH*)handle;
		return FMOD_RESULT::FMOD_OK;
	}
	return ConvertResult(result);
}

static FMOD_RESULT FmodReadFile(void *handle, void *buffer, unsigned int sizebytes, unsigned int *bytesread, void *userdata)
{
	FSResult result = FMODContext::m_pFileSystem->Read(*(FH*)handle, buffer, 1, sizebytes, (FSize*)bytesread);
	return ConvertResult(result);
}

static FMOD_RESULT FmodSeekFile(void *handle, unsigned int pos, void *userdata)
{
	FSResult result = FMODContext::m_pFileSystem->FileSeek(*(FH*)handle, pos, FSSO_SET);
	return ConvertResult(result);
}
#endif

FMODContext::FMODContext(Platform* pPlatform) : AudioContext(pPlatform) {
	m_pFileSystem = pPlatform->GetApplication()->GetFileSystem();
}

FMODContext::~FMODContext()
{
	if (m_pSystem)
	{
		m_pSystem->release();
	}
	
#ifdef _ORBIS
	int result = sceKernelStopUnloadModule(m_ModuleHandle, 0, NULL, 0, NULL, NULL);
	assert(result >= SCE_OK);
#endif
}

void FMODContext::Initialize()
{
#ifdef _ORBIS
	m_ModuleHandle = sceKernelLoadStartModule(FMOD_LIB_PATH, 0, 0, 0, NULL, NULL);
	assert(m_ModuleHandle >= SCE_OK);
#endif

	FMOD_RESULT result = FMOD::System_Create(&m_pSystem);

#if defined(EDITOR) || defined(DEBUG)
	if (result != FMOD_OK)
	{
		m_pLoggingSystem->Log(LOGLEVEL_ERROR, "FMOD", FMOD_ErrorString(result));
		return;
	}
#endif

	result = m_pSystem->init(512, FMOD_INIT_NORMAL, 0);
	
	// IMPLEMENT FILE SYSTEM FUNCTIONS
#ifdef _ORBIS
	m_pSystem->setFileSystem(&FmodOpenFile, &FmodCloseFile, &FmodReadFile, &FmodSeekFile, 0, 0, -1);
#endif

#if defined(EDITOR) || defined(DEBUG)
	if (result != FMOD_OK)
	{
		m_pLoggingSystem->Log(LOGLEVEL_ERROR, "FMOD", FMOD_ErrorString(result));
		return;
	}
#endif

	m_pSystem->set3DSettings(0.0f, 100.0f, 1.0f);
}

void FMODContext::Update()
{
	m_pSystem->update();
}

bool FMODContext::CreateSound(const std::string& soundPath, void*& pSound, bool bIs3D)
{
	FMOD::Sound* pFMODSound;
	FMOD_MODE params = FMOD_LOOP_NORMAL | FMOD_CREATESAMPLE;

	if (bIs3D)
		params |= FMOD_3D;

	FMOD_RESULT result = m_pSystem->createSound(
		(m_pPlatform->GetBasePath() + soundPath).c_str(),
		params,
		0,
		&pFMODSound
	);

	if (result != FMOD_OK)
	{
#if defined(EDITOR) || defined(DEBUG)
		m_pLoggingSystem->Log(LOGLEVEL_ERROR, "FMOD", FMOD_ErrorString(result));
#endif
		return false;
	}

	pSound = pFMODSound;

	return true;
}

void FMODContext::PlaySound(const SoundReference* pSoundReference, void*& pChannel, const Vector3& v3Position, float fVolume, bool bIsPaused)
{
	if (!pSoundReference || !pSoundReference->Sound)
		return;

	FMOD::Channel* pFMODChannel = (FMOD::Channel*)pChannel;

	if (pFMODChannel)
	{
		pFMODChannel->setPaused(bIsPaused);
		return;
	}

	FMOD_RESULT result = m_pSystem->playSound(
		(FMOD::Sound*)pSoundReference->Sound,
		nullptr,
		true,
		&pFMODChannel
	);

	if (result != FMOD_OK)
	{
#if defined(EDITOR) || defined(DEBUG)
		m_pLoggingSystem->Log(LOGLEVEL_ERROR, "FMOD", FMOD_ErrorString(result));
#endif
	}

	// Set mode once, volume
	pFMODChannel->setVolume(fVolume);

	// Set 3D position
	if (pSoundReference->GetRefPath().find("_BGM") == std::string::npos)
	{
		pFMODChannel->setMode(FMOD_3D);

		FMOD_VECTOR soundPosition;
		soundPosition.x = v3Position.x;
		soundPosition.y = v3Position.y;
		soundPosition.z = v3Position.z;

		pFMODChannel->set3DAttributes(&soundPosition, nullptr);
	}

	pFMODChannel->setPaused(bIsPaused);

	pChannel = pFMODChannel;
}

void FMODContext::PauseSound(void* pChannel)
{
	if (!pChannel)
		return;

	FMOD_RESULT result = ((FMOD::Channel*)pChannel)->setPaused(true);
	(void)result;

#if defined(EDITOR) || defined(DEBUG)
	if (result != FMOD_OK)
	{
		m_pLoggingSystem->Log(LOGLEVEL_ERROR, "FMOD", FMOD_ErrorString(result));
	}
#endif
}

void FMODContext::StopSound(void* pChannel)
{
	if (!pChannel)
		return;

	FMOD_RESULT result = ((FMOD::Channel*)pChannel)->stop();
	(void)result;

#if defined(EDITOR) || defined(DEBUG)
	if (result != FMOD_OK)
	{
		m_pLoggingSystem->Log(LOGLEVEL_ERROR, "FMOD", FMOD_ErrorString(result));
	}
#endif
}

void FMODContext::PlayBGM(SoundReference* pSoundReference, float fVolume, uint32_t uiLoopStart, uint32_t uiLoopEnd)
{
	if (!pSoundReference || !pSoundReference->Sound || pSoundReference == m_pBGMReference)
		return;

	fVolume = std::max(0.f, fVolume);

	if (m_pBGMReference)
		m_pBGMReference->Release();

	m_pBGMReference = pSoundReference;
	m_pBGMReference->IncrementRef();

	if (m_pBGMChannel)
	{
		StopSound(m_pBGMChannel);
		m_pBGMChannel = nullptr;
	}

	FMOD_RESULT result = m_pSystem->playSound(
		(FMOD::Sound*)pSoundReference->Sound,
		nullptr,
		true,
		&m_pBGMChannel
	);

	if (result != FMOD_OK)
	{
#if defined(EDITOR) || defined(DEBUG)
		m_pLoggingSystem->Log(LOGLEVEL_ERROR, "FMOD", FMOD_ErrorString(result));
#endif
	}

	// Set loop points
	m_pBGMChannel->setLoopPoints(uiLoopStart, FMOD_TIMEUNIT_PCM, uiLoopEnd, FMOD_TIMEUNIT_PCM);
	
	// Set volume
	m_pBGMChannel->setVolume(fVolume);
	m_pBGMChannel->setPaused(false);

	m_fBGMVolume = fVolume;
}

void FMODContext::ResumeBGM()
{
	if (!m_pBGMChannel)
		return;

	FMOD_RESULT result = m_pBGMChannel->setPaused(false);
	(void)result;

#if defined(EDITOR) || defined(DEBUG)
	if (result != FMOD_OK)
	{
		m_pLoggingSystem->Log(LOGLEVEL_ERROR, "FMOD", FMOD_ErrorString(result));
	}
#endif
}

void FMODContext::PauseBGM()
{
	if (!m_pBGMChannel)
		return;

	FMOD_RESULT result = m_pBGMChannel->setPaused(true);
	(void)result;

#if defined(EDITOR) || defined(DEBUG)
	if (result != FMOD_OK)
	{
		m_pLoggingSystem->Log(LOGLEVEL_ERROR, "FMOD", FMOD_ErrorString(result));
	}
#endif
}

void FMODContext::StopBGM()
{
	if (!m_pBGMChannel)
		return;

	FMOD_RESULT result = m_pBGMChannel->stop();
	(void)result;

	m_pBGMChannel = nullptr;

	m_pBGMReference->Release();
	m_pBGMReference = nullptr;

#if defined(EDITOR) || defined(DEBUG)
	if (result != FMOD_OK)
	{
		m_pLoggingSystem->Log(LOGLEVEL_ERROR, "FMOD", FMOD_ErrorString(result));
	}
#endif
}

void FMODContext::SetBGMVolume(float fVolume)
{
	if (!m_pBGMChannel)
		return;

	fVolume = std::max(0.f, fVolume);

	FMOD_RESULT result = m_pBGMChannel->setVolume(fVolume);
	(void)result;

#if defined(EDITOR) || defined(DEBUG)
	if (result != FMOD_OK)
	{
		m_pLoggingSystem->Log(LOGLEVEL_ERROR, "FMOD", FMOD_ErrorString(result));
	}
#endif

	m_fBGMVolume = fVolume;
}

bool FMODContext::IsBGMPlaying() const
{
	bool b = false;

	if (m_pBGMChannel)
		m_pBGMChannel->isPlaying(&b);

	return b;
}

bool FMODContext::IsPlaying(void* pChannel)
{
	bool bPlaying = false;
	((FMOD::Channel*)pChannel)->isPlaying(&bPlaying);

	return bPlaying;
}

void FMODContext::SetVolume(void* pChannel, float fVolume)
{
	if (!pChannel)
		return;

	FMOD_RESULT result = ((FMOD::Channel*)pChannel)->setVolume(fVolume);
	(void)result;

#if defined(EDITOR) || defined(DEBUG)
	if (result != FMOD_OK)
	{
		m_pLoggingSystem->Log(LOGLEVEL_ERROR, "FMOD", FMOD_ErrorString(result));
	}
#endif
}

float FMODContext::GetPlaybackPosition(void* pChannel)
{
	if (!pChannel)
		return 0.0f;

	unsigned int position;
	((FMOD::Channel*)pChannel)->getPosition(&position, FMOD_TIMEUNIT_MS);

	// ms to s
	return static_cast<float>(position * 0.001f);
}

float FMODContext::GetVolume(void* pChannel) const
{
	if (!pChannel)
		return 1.f;

	float fVolume = 1.f;
	FMOD_RESULT result = ((FMOD::Channel*)pChannel)->getVolume(&fVolume);
	(void)result;

#if defined(EDITOR) || defined(DEBUG)
	if (result != FMOD_OK)
	{
		m_pLoggingSystem->Log(LOGLEVEL_ERROR, "FMOD", FMOD_ErrorString(result));
	}
#endif

	return fVolume;
}

void FMODContext::SetPlaybackPosition(void* pChannel, float position)
{
	if (!pChannel)
		return;

	((FMOD::Channel*)pChannel)->setPosition(static_cast<uint32_t>(position * 1000.0f), FMOD_TIMEUNIT_MS);
}

void FMODContext::Set3DSystemParameters(const Vector3& v3Position, const Vector3& v3Velocity, const Vector3& v3Forward, const Vector3& v3Up)
{
	FMOD_VECTOR soundPosition;
	soundPosition.x = v3Position.x;
	soundPosition.y = v3Position.y;
	soundPosition.z = v3Position.z;

	FMOD_VECTOR soundVelocity;
	soundVelocity.x = v3Position.x;
	soundVelocity.y = v3Position.y;
	soundVelocity.z = v3Position.z;

	FMOD_VECTOR soundForward;
	soundForward.x = v3Forward.x;
	soundForward.y = v3Forward.y;
	soundForward.z = v3Forward.z;

	FMOD_VECTOR soundUp;
	soundUp.x = v3Up.x;
	soundUp.y = v3Up.y;
	soundUp.z = v3Up.z;

	m_pSystem->set3DListenerAttributes(
		0,
		&soundPosition,
		&soundVelocity,
		&soundForward,
		&soundUp
	);
}

float FMODContext::GetLength(const SoundReference* pSoundReference)
{
	unsigned int length;
	((FMOD::Sound*)pSoundReference->Sound)->getLength(
		&length,
		FMOD_TIMEUNIT_MS
	);

	// ms to s
	return static_cast<float>(length * 0.001f);
}

void FMODContext::Set3DParameters(void* pChannel, Vector3 position, Vector3 velocity)
{
	if (!pChannel)
		return;

	FMOD_VECTOR soundPosition;
	soundPosition.x = position.x;
	soundPosition.y = position.y;
	soundPosition.z = position.z;

	FMOD_VECTOR soundVelocity;
	soundVelocity.x = position.x;
	soundVelocity.y = position.y;
	soundVelocity.z = position.z;

	FMOD_RESULT result = ((FMOD::Channel*)pChannel)->set3DAttributes(&soundPosition, &soundVelocity);
	(void)result;

#if defined(EDITOR) || defined(DEBUG)
	if (result != FMOD_OK)
	{
		m_pLoggingSystem->Log(LOGLEVEL_ERROR, "FMOD", FMOD_ErrorString(result));
	}
#endif
}

void FMODContext::GetLoopPoints(void* pChannel, uint32_t& uiLoopStart, uint32_t& uiLoopEnd) const
{
	if (!pChannel)
		return;

	FMOD_RESULT result = ((FMOD::Channel*)pChannel)->getLoopPoints(&uiLoopStart, FMOD_TIMEUNIT_PCM, &uiLoopEnd, FMOD_TIMEUNIT_PCM);
	(void)result;

#if defined(EDITOR) || defined(DEBUG)
	if (result != FMOD_OK)
	{
		m_pLoggingSystem->Log(LOGLEVEL_ERROR, "FMOD", FMOD_ErrorString(result));
	}
#endif
}

void FMODContext::SetLoopPoints(void* pChannel, uint32_t uiLoopStart, uint32_t uiLoopEnd)
{
	if (!pChannel)
		return;

	FMOD_RESULT result = ((FMOD::Channel*)pChannel)->setLoopPoints(uiLoopStart, FMOD_TIMEUNIT_PCM, uiLoopEnd, FMOD_TIMEUNIT_PCM);
	(void)result;

#if defined(EDITOR) || defined(DEBUG)
	if (result != FMOD_OK)
	{
		m_pLoggingSystem->Log(LOGLEVEL_ERROR, "FMOD", FMOD_ErrorString(result));
	}
#endif
}

void FMODContext::SetLoopCount(void* pChannel, int32_t iLoopCount)
{
	if (!pChannel)
		return;

	FMOD_RESULT result = ((FMOD::Channel*)pChannel)->setLoopCount(iLoopCount);
	(void)result;

#if defined(EDITOR) || defined(DEBUG)
	if (result != FMOD_OK)
	{
		m_pLoggingSystem->Log(LOGLEVEL_ERROR, "FMOD", FMOD_ErrorString(result));
	}
#endif
}