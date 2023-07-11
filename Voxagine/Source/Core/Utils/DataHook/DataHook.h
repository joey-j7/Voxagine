#pragma once

#include <unordered_map>
#include <string>
#include <vector>

#include "Core/System/FileSystem.h"

#define USE_DATAHOOKS

#define DATAHOOK_PUSH
#define DATAHOOK_GETVALUE
#define DATAHOOK_INCREMENTVAL
#define DATAHOOK_ADDTOAVERAGE
#define DATAHOOK_FLUSH_TO_FILE

#ifdef USE_DATAHOOKS

class DataHook
{
public:

	static void PushData(const std::string& name, const float data);
	
	static float GetDataValue(const std::string& name);
	
	static void IncrementValue(const std::string& name, float amount = 1.f);

	static void AddToAverage(const std::string& name, float value);

	static void FlushToFile(FileSystem* pFileSystem, const std::string& path);

private:

	static DataHook& Instance();

	std::unordered_map<std::string, float> m_data;

	std::unordered_map<std::string, std::vector<float>> m_averages;
};

#define DATAHOOK_PUSH( name, data ) DataHook::PushData(name, data)
#define DATAHOOK_GETVALUE( name ) DataHook::GetDataValue(name)
#define DATAHOOK_INCREMENTVAL( ... ) DataHook::IncrementValue(__VA_ARGS__)
#define DATAHOOK_ADDTOAVERAGE( name, val ) DataHook::AddToAverage( name, val )
#define DATAHOOK_FLUSH_TO_FILE( filesys, path ) DataHook::FlushToFile( filesys, path )
#endif // USE_DATAHOOKS
