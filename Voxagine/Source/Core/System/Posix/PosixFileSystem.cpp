#include "pch.h"
#include "PosixFileSystem.h"

#include "Core/Application.h"
#include "Core/Threading/JobManager.h"

#include <cerrno>
#include <cinttypes>
#include <cstring>

uint32_t PosixFileSystem::m_FileHandleCtr = INVALID_FH;

void PosixFileSystem::Initialize()
{
	printf("POSIX file system Initialized\n");
}

void PosixFileSystem::Deinitialize()
{
	std::unordered_map<FH, FileInfo>::iterator it;

	for (it = m_FileMap.begin(); it != m_FileMap.end(); it++)
	{
		fclose(it->second.pFile);
	}

	printf("POSIX file system Deinitialized\n");
}

PosixFileSystem::PosixFileSystem(Application* pApp):
	FileSystem(pApp)
{
	m_JobQueueHandle = pApp->GetJobManager().CreateJobQueue();
}

FH PosixFileSystem::OpenFile(const char* pFilePath, FSOpenFlags openFlags)
{
	//Convert FSOpenFlags enum to fopen mode parameter
	std::string openMode = FlagsToOpenMode(openFlags);

	// Open file
	FILE* pFile = fopen(pFilePath, openMode.c_str());
	if (pFile == nullptr)
	{
		printf("Failed to open file with error: %s\n", strerror(errno));
		return INVALID_FH;
	}

	//Insert opened file to file handle storage
	FH fileHandle = ++m_FileHandleCtr;
	m_FileMap[fileHandle] = FileInfo(pFile, pFilePath, openFlags);

	//Return file handle for future user operations
	return fileHandle;
}

FSResult PosixFileSystem::CloseFile(FH fileHandle)
{
	if (IsHandleValid(fileHandle))
	{
		fclose(m_FileMap[fileHandle].pFile);
		m_FileMap.erase(fileHandle);
		return FSR_OK;
	}

	printf("Failed to close file, invalid handle: %" PRIu32 "\n", fileHandle);
	return FSR_INVALID_HANDLE;
}

FSResult PosixFileSystem::Read(FH fileHandle, void* pReadBuff, FSize elementSize, FSize length, FSize* pBytesRead)
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
				printf("Failed to read %" PRIu64 " bytes\nActual bytes read %" PRIu64 "\n", length, readSize);
				return FSR_ERR_EOF;
			}
			return FSR_OK;
		}

		printf("Failed to read file, invalid open flags for handle: %" PRIu32 "\n", fileHandle);
		return FSR_INVALID_OPEN_FLAGS;
	}

	printf("Failed to read file, invalid handle: %" PRIu32 "\n", fileHandle);
	return FSR_INVALID_HANDLE;
}

FSResult PosixFileSystem::Write(FH fileHandle, const void* pWriteBuff, FSize elementSize, FSize length)
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

		printf("Failed to write file, invalid open flags for handle: %" PRIu32 "\n", fileHandle);
		return FSR_INVALID_OPEN_FLAGS;
	}

	printf("Failed to write file, invalid handle: %" PRIu32 "\n", fileHandle);
	return FSR_INVALID_HANDLE;
}

FSize PosixFileSystem::GetFileSize(FH fileHandle)
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

	printf("Failed to get file size, invalid handle: %" PRIu32 "\n", fileHandle);
	return 0;
}

FSize PosixFileSystem::FileTell(FH fileHandle)
{
	if (IsHandleValid(fileHandle))
	{
		FILE* pFile = m_FileMap[fileHandle].pFile;
		return ftell(pFile);
	}

	printf("Failed to get current indicator value, invalid handle: %" PRIu32 "\n", fileHandle);
	return 0;
}

FSResult PosixFileSystem::FileSeek(FH fileHandle, FSize offset, FSSeekOrigin origin, FSize* pSeekPos)
{
	if (IsHandleValid(fileHandle))
	{
		FILE* pFile = m_FileMap[fileHandle].pFile;

		/* FSSeekOrigin is an engine enum; do not assume it matches SEEK_*. */
		int iOrigin = SEEK_SET;
		switch (origin)
		{
		case FSSO_SET: iOrigin = SEEK_SET; break;
		case FSSO_CUR: iOrigin = SEEK_CUR; break;
		case FSSO_END: iOrigin = SEEK_END; break;
		}

		if (fseek(pFile, static_cast<long>(offset), iOrigin) != 0)
		{
			printf("Failed to seek file %" PRIu32 ": %s\n", fileHandle, strerror(errno));
			return FSR_FAILED;
		}

		/* Report where we actually landed. The Windows implementation returned
		   fseek's status here, which is always 0 on success. */
		if (pSeekPos != nullptr)
			*pSeekPos = static_cast<FSize>(ftell(pFile));

		return FSR_OK;
	}

	printf("Failed to seek file, invalid handle: %" PRIu32 "\n", fileHandle);
	return FSR_INVALID_HANDLE;
}

FSResult PosixFileSystem::ReadAsync(FSAsyncReadInfo* pInfo)
{
	if (IsHandleValid(pInfo->FileHandle))
	{
		m_pApp->GetJobManager().GetJobQueue(m_JobQueueHandle)->EnqueueWithType<FSResult>([this, pInfo]()
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

std::string PosixFileSystem::FlagsToOpenMode(FSOpenFlags openFlags)
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

bool PosixFileSystem::IsHandleValid(FH fileHandle)
{
	return m_FileMap.find(fileHandle) != m_FileMap.end();
}
