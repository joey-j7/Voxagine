#include "pch.h"

#ifdef _WINDOWS

#include "WINFileSystem.h"
#include "Core/Application.h"
#include "Core/Threading/JobManager.h"

uint32_t WINFileSystem::m_FileHandleCtr = INVALID_FH;

void WINFileSystem::Initialize()
{
	printf("Windows file system Initialized\n");
}

void WINFileSystem::Deinitialize()
{
	std::unordered_map<FH, FileInfo>::iterator it;

	for (it = m_FileMap.begin(); it != m_FileMap.end(); it++)
	{
		fclose(it->second.pFile);
	}

	printf("Windows file system Deinitialized\n");
}

WINFileSystem::WINFileSystem(Application* pApp):
	FileSystem(pApp)
{
	m_JobQueueHandle = pApp->GetJobManager().CreateJobQueue();
}

FH WINFileSystem::OpenFile(const char* pFilePath, FSOpenFlags openFlags)
{
	//Convert FSOpenFlags enum to fopen mode parameter
	std::string openMode = FlagsToOpenMode(openFlags);

	// Open file
	FILE* pFile;
	errno_t err = fopen_s(&pFile, pFilePath, openMode.c_str());
	if (err != 0)
	{
		char errmsg[256];
		strerror_s(errmsg, 256, err);
		printf("Failed to open file with error: %s\n", errmsg);
		return INVALID_FH;
	}

	//Insert opened file to file handle storage
	FH fileHandle = ++m_FileHandleCtr;
	m_FileMap[fileHandle] = FileInfo(pFile, pFilePath, openFlags);

	//Return file handle for future user operations
	return fileHandle;
}

FSResult WINFileSystem::CloseFile(FH fileHandle)
{
	if (IsHandleValid(fileHandle))
	{
		fclose(m_FileMap[fileHandle].pFile);
		m_FileMap.erase(fileHandle);
		return FSR_OK;
	}
	
	printf("Failed to close file, invalid handle: %I32i\n", fileHandle);
	return FSR_INVALID_HANDLE;
}

FSResult WINFileSystem::Read(FH fileHandle, void* pReadBuff, FSize elementSize, FSize length, FSize* pBytesRead)
{
	if (IsHandleValid(fileHandle))
	{
		FSOpenFlags flags = m_FileMap[fileHandle].OpenFlags;
		FILE* pFile = m_FileMap[fileHandle].pFile;

		if (flags & FSOpenFlags::FSOF_READ || flags & FSOpenFlags::FSOF_RDWR)
		{
			FSize readSize = fread(pReadBuff, elementSize, length, pFile);
			if (pBytesRead != nullptr)
			{
				*pBytesRead = readSize;
			}

			if (readSize != length)
			{
				printf("Failed to read %I64i bytes\nActual bytes read %I64i\n", length, readSize);
				return FSR_ERR_EOF;
			}
			return FSR_OK;
		}
		
		printf("Failed to read file, invalid open flags for handle: %I32i\n", fileHandle);
		return FSR_INVALID_OPEN_FLAGS;
	}

	printf("Failed to read file, invalid handle: %I32i\n", fileHandle);
	return FSR_INVALID_HANDLE;
}

FSResult WINFileSystem::Write(FH fileHandle, const void* pWriteBuff, FSize elementSize, FSize length)
{
	if (IsHandleValid(fileHandle))
	{
		FSOpenFlags flags = m_FileMap[fileHandle].OpenFlags;
		FILE* pFile = m_FileMap[fileHandle].pFile;

		if (flags & FSOpenFlags::FSOF_WRITE || flags & FSOpenFlags::FSOF_RDWR)
		{
			fwrite(pWriteBuff, elementSize, length, pFile);
			return FSR_OK;
		}

		printf("Failed to write file, invalid open flags for handle: %I32i\n", fileHandle);
		return FSR_INVALID_OPEN_FLAGS;
	}
	
	printf("Failed to write file, invalid handle: %I32i\n", fileHandle);
	return FSR_INVALID_HANDLE;
}

FSize WINFileSystem::GetFileSize(FH fileHandle)
{
	if (IsHandleValid(fileHandle))
	{
		//Get file size
		FILE* pFile = m_FileMap[fileHandle].pFile;
		fseek(pFile, 0, SEEK_END);
		FSize size = ftell(pFile);
		rewind(pFile);

		return size;
	}
	
	printf("Failed to get file size, invalid handle: %I32i", fileHandle);
	return 0;
}

FSize WINFileSystem::FileTell(FH fileHandle)
{
	if (IsHandleValid(fileHandle))
	{
		//Get file size
		FILE* pFile = m_FileMap[fileHandle].pFile;
		return ftell(pFile);
	}
	
	printf("Failed to get current indicator value, invalid handle: %I32i", fileHandle);
	return 0;
}

FSResult WINFileSystem::FileSeek(FH fileHandle, FSize offset, FSSeekOrigin origin, FSize* pSeekPos)
{
	if (IsHandleValid(fileHandle))
	{
		//Get file size
		FILE* pFile = m_FileMap[fileHandle].pFile;
		FSize seekPos = fseek(pFile, offset, origin);
		if (pSeekPos != nullptr)
			*pSeekPos = seekPos;
		return FSR_OK;
	}

	printf("Failed to seek file, invalid handle: %I32i", fileHandle);
	return FSR_INVALID_HANDLE;
}

FSResult WINFileSystem::ReadAsync(FSAsyncReadInfo* pInfo)
{
	if (IsHandleValid(pInfo->FileHandle))
	{
		m_pApp->GetJobManager().GetJobQueue(m_JobQueueHandle)->EnqueueWithType<FSResult>([&]()
		{
			return Read(pInfo->FileHandle, pInfo->pReadBuffer, pInfo->ElementSize, pInfo->ReadLength, &pInfo->BytesRead);
		},
		[pInfo](FSResult result)
		{
			pInfo->Done(pInfo, result);
		}, JT_IO);

		return FSR_OK;
	}

	return FSR_INVALID_HANDLE;
}

std::string WINFileSystem::FlagsToOpenMode(FSOpenFlags openFlags)
{
	std::string flags = "";
	if (openFlags & FSOpenFlags::FSOF_READ && openFlags & FSOpenFlags::FSOF_APPEND)
		flags = "a+";
	else if (openFlags & FSOpenFlags::FSOF_READ)
		flags = "r";
	else if (openFlags & FSOpenFlags::FSOF_WRITE && openFlags & FSOpenFlags::FSOF_APPEND)
		flags = "a";
	else if (openFlags & FSOpenFlags::FSOF_WRITE)
		flags = "w";
	else if (openFlags & FSOpenFlags::FSOF_RDWR && openFlags & FSOpenFlags::FSOF_CREATE)
		flags = "w+";
	else if (openFlags & FSOpenFlags::FSOF_RDWR)
		flags = "r+";

	if (openFlags & FSOpenFlags::FSOF_BINARY)
		flags += "b";

	return flags;
}

bool WINFileSystem::IsHandleValid(FH fileHandle)
{
	return m_FileMap.find(fileHandle) != m_FileMap.end();
}

#endif