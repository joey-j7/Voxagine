#include "pch.h"

#include "Core/Resources/Formats/FMODSoundReference.h"

/* Silent implementation of FMODSoundReference, compiled instead of
 * FMODSoundReference.cpp when VOXAGINE_ENABLE_FMOD is off.
 *
 * ResourceManager names the type concretely, so the class has to exist with
 * the same name; only its behaviour changes. Load reports success so callers
 * treat sounds as present and the reference-counting in ReferenceManager
 * behaves normally. */

FMODSoundReference::~FMODSoundReference()
{
	Free();
}

bool FMODSoundReference::Load(const std::string& filePath)
{
	VX_UNUSED(filePath);

	Sound = nullptr;

	return true;
}

void FMODSoundReference::Free()
{
	Sound = nullptr;
}
