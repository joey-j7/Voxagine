#pragma once

#include "Core/System/FileSystem.h"

#include <cstdio>
#include <unordered_map>

struct FileInfo
{
	FileInfo()
	{
		pFile = nullptr;
		FilePath = "";
		OpenFlags = static_cast<FSOpenFlags>(0);
	}
	FileInfo(FILE* _pFile, std::string _filePath, FSOpenFlags openFlags):
		pFile(_pFile),
		FilePath(_filePath),
		OpenFlags(openFlags) {}

	FILE* pFile;
	std::string FilePath;
	FSOpenFlags OpenFlags;
};

class PosixFileSystem : public FileSystem
{
public:
	PosixFileSystem(Application* pApp);

	virtual FH OpenFile(const char* pFilePath, FSOpenFlags openFlags) override;
	virtual FSResult CloseFile(FH fileHandle) override;
	virtual FSResult Read(FH fileHandle, void* pReadBuff, FSize elementSize, FSize length, FSize* pBytesRead = nullptr) override;
	virtual FSResult ReadAsync(FSAsyncReadInfo* pInfo) override;
	virtual FSResult Write(FH fileHandle, const void* pWriteBuff, FSize elementSize, FSize length) override;
	virtual FSize GetFileSize(FH fileHandle) override;
	virtual FSize FileTell(FH fileHandle) override;
	virtual FSResult FileSeek(FH fileHandle, FSize offset, FSSeekOrigin origin, FSize* pSeekPos = nullptr) override;

protected:
	virtual void Initialize() override;
	virtual void Deinitialize() override;

private:
	static uint32_t m_FileHandleCtr;

	std::unordered_map<FH, FileInfo> m_FileMap;

	std::string FlagsToOpenMode(FSOpenFlags openFlags);
	bool IsHandleValid(FH fileHandle);
};
