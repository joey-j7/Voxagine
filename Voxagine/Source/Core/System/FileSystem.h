#pragma once
#include <stdint.h>
#include <string>

// File handle
typedef uint32_t FH;
typedef uint64_t FSize;

#define INVALID_FH 0

enum FSResult
{
	FSR_OK,
	FSR_FAILED,
	FSR_INVALID_HANDLE,
	FSR_INVALID_OPEN_FLAGS,
	FSR_ERR_EOF
};

enum FSOpenFlags
{
	FSOF_READ = 1u << 0,
	FSOF_WRITE = 1u << 1,
	FSOF_RDWR = (FSOF_READ | FSOF_WRITE),
	FSOF_APPEND = 1u << 2,
	FSOF_CREATE = 1u << 3,
	FSOF_BINARY = 1u << 4
};

inline FSOpenFlags operator|(FSOpenFlags a, FSOpenFlags b)
{
	return static_cast<FSOpenFlags>(static_cast<int>(a) | static_cast<int>(b));
}

enum FSSeekOrigin
{
	FSSO_SET,	/* Beginning of file */
	FSSO_CUR,	/* Current position of the file pointer */
	FSSO_END	/* End of file */
};

typedef struct FSAsyncReadInfo FSAsyncReadInfo;
typedef std::function<void(FSAsyncReadInfo*, FSResult)> FSAsyncReadCallback;

struct FSAsyncReadInfo
{
	FH FileHandle = INVALID_FH; 
	void* pReadBuffer = nullptr;
	FSize ElementSize = 0;
	FSize ReadLength = 0;
	FSize BytesRead = 0;

	FSAsyncReadCallback Done;
};

class Application;
class FileSystem
{
	friend class Application;

public:
	FileSystem(Application* pApp) 
	{ 
		m_pApp = pApp;
	}
	virtual ~FileSystem() {}

	virtual FH OpenFile(const char* pFilePath, FSOpenFlags openFlags) = 0;
	virtual FSResult CloseFile(FH fileHandle) = 0;
	virtual FSResult Read(FH fileHandle, void* pReadBuff, FSize elementSize, FSize length, FSize* pBytesRead = nullptr) = 0;
	virtual FSResult ReadAsync(FSAsyncReadInfo* pInfo) = 0;
	virtual FSResult Write(FH fileHandle, const void* pWriteBuff, FSize elementSize, FSize length) = 0;
	virtual FSize GetFileSize(FH fileHandle) = 0;
	virtual FSize FileTell(FH fileHandle) = 0;
	virtual FSResult FileSeek(FH fileHandle, FSize offset, FSSeekOrigin origin, FSize* pSeekPos = nullptr) = 0;

protected:
	Application* m_pApp;
	uint32_t m_JobQueueHandle;

	virtual void Initialize() = 0;
	virtual void Deinitialize() = 0;
};