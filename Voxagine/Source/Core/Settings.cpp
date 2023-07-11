#include "pch.h"
#include "Core/Settings.h"

#include <External/rttr/registration>
#include <External/rttr/policy.h>

RTTR_REGISTRATION
{
	rttr::registration::enumeration<PlatformType>("E_PlatformType")
	(
		rttr::value("PT_WINDOWS", PlatformType::PT_WINDOWS),
		rttr::value("PT_SWITCH", PlatformType::PT_SWITCH),
		rttr::value("PT_ORBIS", PlatformType::PT_ORBIS),
		rttr::value("PT_ANDROID", PlatformType::PT_ANDROID)
	);

	rttr::registration::enumeration<AudioAPI>("E_AudioAPI")
	(
		rttr::value("AA_FMOD", AudioAPI::AA_FMOD),
		rttr::value("AA_OPENAL", AudioAPI::AA_OPENAL)
	);

	rttr::registration::enumeration<RenderingAPI>("E_RenderingAPI")
	(
		rttr::value("RA_DIRECTX12", RenderingAPI::RA_DIRECTX12),
		rttr::value("RA_OPENGL", RenderingAPI::RA_OPENGL),
		rttr::value("RA_OPENGLES", RenderingAPI::RA_OPENGLES),
		rttr::value("RA_VULKAN", RenderingAPI::RA_VULKAN),
		rttr::value("RA_ORBIS", RenderingAPI::RA_ORBIS)
	);

	rttr::registration::class_<Settings>("Settings")
		.constructor<>()(rttr::policy::ctor::as_object)
		.property("Title", &Settings::m_Title)
		.property("WorldFileExtension", &Settings::m_WorldFileExtension)
		.property("EngineAssetsPath", &Settings::m_EngineAssetsPath)
		.property("FontPath", &Settings::m_FontPath)
		.property("PlatformType", &Settings::m_PlatformType)
		.property("RenderApiType", &Settings::m_RenderApiType)
		.property("AudioApiType", &Settings::m_AudioApiType)
		.property("FixedTimeStep", &Settings::m_dFixedTimeStep)
		.property("FrameLimit", &Settings::m_dFrameLimit)
		.property("EnableVSync", &Settings::IsVSyncEnabled, &Settings::SetVSync)
		.property("FXAAEnabled", &Settings::IsFXAAEnabled, &Settings::SetFXAA)
		.property("ShadowsEnabled", &Settings::IsShadowEnabled, &Settings::SetShadowEnabled)
		.property("ResolutionScale", &Settings::GetResolutionScale, &Settings::SetResolutionScale)
		.property("Fullscreen", &Settings::IsFullscreen, &Settings::SetFullscreen)
		.property("InitialWindowSize", &Settings::m_v2InitialWindowSize);
}

void Settings::SetFullscreen(bool bFullscreen)
{
	m_bFullscreen = bFullscreen;
	FullscreenChanged(bFullscreen);
}