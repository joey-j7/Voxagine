#pragma once
#include <string>
#include "Core/Resources/ReferenceManager.h"

#include "Core/Resources/Formats/VoxModel.h"
#include "Core/Resources/Formats/FMODSoundReference.h"
#include "Core/Resources/Formats/TextureReference.h"

class Application;
class ResourceManager
{
public:
	ResourceManager(Application* pApp);

	void Unload();

	TextureReference* LoadTexture(const std::string& filePath);
	FMODSoundReference* LoadSound(const std::string& filePath);
	VoxModel* LoadVox(const std::string& filePath);
	std::vector<VoxModel*> LoadVoxBatch(const std::string& filePath, const std::string& fileName);
	
	VoxModel* CreateHollowVox(const std::string& filePath);

	void GetResourceFilePaths(const std::string fileExtension, std::vector<std::string>& resourceFilePaths);

private:
	void LogFailedLoadResourceMessage(const std::string& resourceTypeName, const std::string& filePath);

private:
	Application* m_pApp;

	ReferenceManager<VoxModel> m_voxManager;
	ReferenceManager<FMODSoundReference> m_soundManager;
	ReferenceManager<TextureReference> m_textureManager;
};