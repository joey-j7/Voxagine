#include "pch.h"
#include "AudioContext.h"

#include <Core/Application.h>

AudioContext::AudioContext(Platform* pPlatform)
{
	m_pPlatform = pPlatform;
	m_pLoggingSystem = &pPlatform->GetApplication()->GetLoggingSystem();
};
