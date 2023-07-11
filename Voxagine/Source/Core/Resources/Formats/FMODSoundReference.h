#pragma once

#include "Core/Resources/Formats/SoundReference.h"

#include "External/FMOD/fmod.hpp"
#include "External/FMOD/fmod_common.h"

class FMODSoundReference : public SoundReference
{
public:
	FMODSoundReference(const std::string& filePath) : SoundReference(filePath) {};
	virtual ~FMODSoundReference();

	virtual bool Load(const std::string& filePath) override;
	virtual void Free() override;
};