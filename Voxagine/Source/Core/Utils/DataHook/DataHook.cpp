
#include "pch.h"

#include "Core/Utils/DataHook/DataHook.h"

DataHook& DataHook::Instance()
{
	static DataHook singletonInstance;
	return singletonInstance;
}

void DataHook::PushData(const std::string& name, const float data)
{
	auto& instance = Instance();

	instance.m_data[name] = data;
}

float DataHook::GetDataValue(const std::string& name)
{
	auto& instance = Instance();

	return instance.m_data[name];
}

void DataHook::IncrementValue(const std::string& name, float amount /*= 1.f*/)
{
	auto& instance = Instance();

	instance.m_data[name] += amount;
}

void DataHook::AddToAverage(const std::string& name, float value)
{
	auto& instance = Instance();

	instance.m_averages[name].push_back(value);
}

void DataHook::FlushToFile(FileSystem* pFileSystem, const std::string& path)
{
	auto& instance = Instance();

	FH handle = pFileSystem->OpenFile(path.c_str(), FSOpenFlags::FSOF_WRITE | FSOpenFlags::FSOF_BINARY | FSOpenFlags::FSOF_APPEND);

	std::string newData = "";
	for ( auto& it : instance.m_data )
	{
		newData = it.first + "," + std::to_string(it.second);
		pFileSystem->Write(handle, &newData[0], sizeof(char), newData.length());
	}

	instance.m_data.clear();

	float total;
	for (auto& it : instance.m_averages)
	{
		total = 0.f;
		for (auto& val : it.second)
		{
			total += val;
		}

		newData = it.first + "," + std::to_string( total / it.second.size() );
		pFileSystem->Write(handle, &newData[0], sizeof(char), newData.length());
	}

	instance.m_averages.clear();

	pFileSystem->CloseFile(handle);
}
