#pragma once

#include <External/nativefiledialog/nfd.h>
#include <External/teenypath/teenypath.h>

#include <string>
#include <vector>

struct FolderTree
{
	TeenyPath::path FolderPath;
	std::vector<std::string> FilePaths;
	std::vector<FolderTree> SubFolders;

	void GetFilteredFiles(std::vector<std::string>& filteredFilePaths, std::string extensionFilter = std::string())
	{
		for (std::string& FileIt : FilePaths)
		{
			if (extensionFilter.empty())
			{
				filteredFilePaths.push_back(FileIt);
			}
			else
			{
				size_t findExtensionFilter = FileIt.find(extensionFilter);
				size_t firstExtensionIndicator = FileIt.find(".");

				if (findExtensionFilter != std::string::npos && findExtensionFilter == firstExtensionIndicator)
					filteredFilePaths.push_back(FileIt);
			}
		}

		for (FolderTree& FolderIt : SubFolders)
		{
			FolderIt.GetFilteredFiles(filteredFilePaths, extensionFilter);
		};
	};
};

class FileBrowser
{
public:
	bool SaveFile(std::string& savePath, const std::string& filters = "json");
	bool OpenFile(std::string& filePath, const std::string& filters = "json");
	bool DeleteFileFromHardDisk(std::string& filePath);

	bool GetFolderTreeInformation(TeenyPath::path folderPath, FolderTree& folderTree, std::vector<std::string> exstensionFilter = std::vector<std::string>());
	bool GetFolderTreeInformation(FolderTree& folderTree, std::vector<std::string> exstensionFilter = std::vector<std::string>());

	std::string ResolveFilePath(const std::string& filePath);
	bool AbsoluteToRelative(const std::string& absolutePath, std::string& relativePath, const std::string& basePath);
};