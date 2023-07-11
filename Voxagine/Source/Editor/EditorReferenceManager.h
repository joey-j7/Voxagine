#pragma once

#include <unordered_map>
#include <string>
#include <functional>
#include <vector>

class Editor;
class ReferenceObject;

class EditorReferenceManager
{
private:
	struct ResourceInformation
	{
		std::vector<ReferenceObject*>* ResourceList;
		std::function<ReferenceObject*(std::string)> LoadFunction;
	};

public:
	EditorReferenceManager();
	~EditorReferenceManager();

	void Initialize(Editor* pEditor);
	void UnInitialize();

	void ImportFile(std::string filePath);
	void ImportFiles(const std::vector<std::string>& filePathBatch);
	bool HasFile(std::string filePath) const;

private:
	void RegisterResourceInformation(std::string fileExtension, std::function<ReferenceObject*(std::string)> loadFunction);
	std::string FileGetExtension(std::string filePath) const;

	const ResourceInformation* GetResourceInformation(std::string filePath) const;
private:
	Editor* m_pEditor = nullptr;

	std::unordered_map<std::string, ResourceInformation> m_ResourceLookUp;
	std::unordered_map<std::string, std::vector<ReferenceObject*>> m_ResourceReferences;
};