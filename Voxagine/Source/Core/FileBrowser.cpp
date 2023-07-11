#include "pch.h"
#include "FileBrowser.h"

#include <stdio.h>

bool FileBrowser::SaveFile(std::string& savePath, const std::string& filters)
{
	savePath = "";
	char *outPath = nullptr;
	nfdresult_t result = NFD_SaveDialog(filters.c_str(), nullptr, &outPath);

	if (result == NFD_OKAY) {
		savePath = std::string(outPath);
		free(outPath);
		return true;
	}
	else if (result == NFD_CANCEL) 
		return false;
	return false;
}

bool FileBrowser::OpenFile(std::string& filePath, const std::string& filters)
{
	char *outPath = nullptr;
	nfdresult_t result = NFD_OpenDialog(filters.c_str(), nullptr, &outPath);

	if (result == NFD_OKAY) {
/*		filePath = std::string(outPath);*/

		TeenyPath::path TempPath = outPath;
		filePath = TempPath.resolve_absolute().generic_string();

		free(outPath);
		return true;
	}
	else if (result == NFD_CANCEL)
		return false;
	return false;
}

bool FileBrowser::DeleteFileFromHardDisk(std::string & filePath)
{
	return remove(filePath.c_str());
}

bool FileBrowser::GetFolderTreeInformation(TeenyPath::path folderPath, FolderTree & folderTree, std::vector<std::string> exstensionFilter)
{
	folderTree.FolderPath = folderPath;
	return GetFolderTreeInformation(folderTree, exstensionFilter);
}

bool FileBrowser::GetFolderTreeInformation(FolderTree & folderTree, std::vector<std::string> exstensionFilter)
{
	if (!folderTree.FolderPath.is_directory())
		return false;

	std::vector<TeenyPath::path> SubDirOrFiles = ls(folderTree.FolderPath);

	if (SubDirOrFiles.empty())
		return true;


	for (TeenyPath::path& SubDirOrFile : SubDirOrFiles)
	{
		if (SubDirOrFile.is_lexically_normal())
		{
			if (SubDirOrFile.is_directory())
			{
				FolderTree SubDirtree;
				SubDirtree.FolderPath = SubDirOrFile;
				folderTree.SubFolders.push_back(SubDirtree);
				GetFolderTreeInformation(folderTree.SubFolders.back());
			}

			if (SubDirOrFile.is_regular_file())
			{
				std::vector<std::string>::iterator found = std::find(exstensionFilter.begin(), exstensionFilter.end(), SubDirOrFile.extension());

				if (found != exstensionFilter.end() || exstensionFilter.empty())
					folderTree.FilePaths.push_back(SubDirOrFile.generic_string());
			}
		}
	}

	return true;
}

std::string FileBrowser::ResolveFilePath(const std::string & filePath)
{
	TeenyPath::path resolvedFilePath = filePath;
	return resolvedFilePath.generic_string();
}

bool FileBrowser::AbsoluteToRelative(const std::string& absolutePath, std::string& relativePath, const std::string& basePath)
{
	std::size_t pos = absolutePath.find(basePath);
	if (pos != std::string::npos)
	{
		relativePath = absolutePath.substr(pos);
		return true;
	}

	return false;
}