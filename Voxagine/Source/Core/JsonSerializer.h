#pragma once
#include <functional>
#include <unordered_map>

#include <External/rttr/type>

#include <External/rapidjson/document.h>
#include <External/rapidjson/writer.h>
#include <External/rapidjson/filewritestream.h>
#include <External/rapidjson/filereadstream.h>
#include <External/rapidjson/prettywriter.h>

#include "Core/LoggingSystem/LoggingSystem.h"
#include "Core/System/FileSystem.h"

using namespace rapidjson;

class World;
class Entity;
class Component;
class Settings;
class FileSystem;
class JsonSerializer
{
public:
	JsonSerializer(Settings& settings, LoggingSystem& logger);

	void Initialize(FileSystem* pFileSystem);

	bool SerializeWorld(World* pWorld, Document& worldDoc);
	void SerializeWorldAsync(World* pWorld, std::function<void(bool, Document&)> callback);
	void SerializeWorldToFile(const std::string& filePath, World* pWorld, std::function<void(bool)> callback);

	bool DeserializeWorld(World& world, Document& worldDoc);
	bool DeserializeWorldFromFile(World& world, const std::string& filePath);

	bool SerializeEntityToFile(const std::string& filePath, Entity* pEntity);
	Entity* DeserializeEntityFromFile(const std::string& filePath, World& world);

	Entity* ValueToEntity(Value& val, World& world, bool bGenerateNewId = false, Entity* pParent = nullptr);
	Component* ValueToComponent(World& world, Value& val, Entity* pEntity);
	void ComponentToValue(Component* pComponent, Value& val, Document::AllocatorType& alloc);
	void EntityToValue(Entity* pEntity, Value& val, Document::AllocatorType& alloc);

	//Adds the root entity and all its children to the world
	void AddRootEntityToWorld(World& world, Entity* pEntity);

	// Fill type with data from JSON document
	template<typename T>
	void FromJson(T& instance, Document& doc);

	// Fill type with data from file on disk
	template<typename T>
	bool FromJsonFile(T& instance, const std::string& filePath);

	// Write data from type to disk
	template<typename T>
	bool ToJsonFile(T& instance, const std::string& filePath, bool bPretty = false);

	// Write data from type to JSON document
	void ToJson(rttr::instance instance, Document& doc);

	/**
	 * @brief - resolve the connection where we have vectors/arrays/properties
	 * 
	 * @param pWorld - World that needs to be reconnected.
	*/
	void ResolveWorldLinks(World& pWorld);

private:
	bool SetInstanceArrayProperty(rttr::instance& instance, const rttr::property& property, rttr::variant& variant, const rttr::type& variantType, const int& index);
	bool SetInstanceProperty(rttr::instance& instance, const rttr::property& property, rttr::variant& variant, const rttr::type& variantType, bool bIsSequentialContainer = false);

	// Keep reference for prefab id
	std::unordered_map<int64_t, int64_t> m_vOldPrefabIDs = {};

	//Change version to 1.10 when using chunk system
	const double WORLD_VERSION = 1.10;

	FileSystem* m_pFileSystem = nullptr;
	Settings& m_Settings;
	LoggingSystem& m_Logger;

	void AddPropertyAsMember(Value& val, rttr::property& prop, rttr::instance& instance, Document::AllocatorType& alloc);
	void SetPropertyFromValue(World* world, const Value& val, rttr::property& prop, rttr::instance& instance);

	//Stores entities in chunks based on their position and bounds
	//Chunks are stored from bottom to top and left to right
	void ChunkifyWorld(World& world, Value& chunkDataVal, UVector2 chunkSize, Document::AllocatorType& alloc);
	void VariantToValue(Value& val, const rttr::variant& variant, Document::AllocatorType& alloc);
	void ValueToVariant(World* world, rttr::instance& instance, rttr::variant& variant, const Value& val, rttr::property& property, const rttr::type& variantType);
	void ValueToVariant(World* world, rttr::instance& instance, rttr::variant& variant, const Value& val, rttr::property& property, const rttr::type& variantType, const int& index);
	void ContainerValueToVariant(World* world, rttr::instance& instance, rttr::variant& variant, const Value& val, rttr::property& property, const rttr::type& variantType, rttr::variant& containerVar);

	uint64_t GetHighestEntityID(Value& rootEntityVal);
};

template<typename T>
void JsonSerializer::FromJson(T& instance, Document& doc)
{
	if (doc.IsObject())
	{
		if (!doc.HasMember("Data") && !doc.HasMember("Type"))
		{
			m_Logger.Log(LOGLEVEL_ERROR, "JsonSerializer", "Failed parse document to class");
			return;
		}

		Value& classVal = doc["Data"];
		std::string className = doc["Type"].GetString();
		rttr::type classType = rttr::type::get_by_name(className);
		rttr::instance classInstance = instance;

		if (!classInstance.is_valid())
		{
			m_Logger.Log(LOGLEVEL_ERROR, "JsonSerializer", "Failed to parse class with name: " + className);
			return;
		}

		for (rttr::property prop : classType.get_properties())
		{
			if (classVal.HasMember(prop.get_name().to_string()))
			{
				SetPropertyFromValue(nullptr, classVal[prop.get_name().to_string()], prop, classInstance);
			}
		}

		return;
	}
}

template<typename T>
bool JsonSerializer::FromJsonFile(T& instance, const std::string& filePath)
{
	FH handle = m_pFileSystem->OpenFile(filePath.c_str(), FSOpenFlags::FSOF_READ);
	if (handle == INVALID_FH)
	{
		return false;
	}

	FSize buffSize = m_pFileSystem->GetFileSize(handle);
	std::vector<char> readBuffer;
	readBuffer.resize(buffSize);
	m_pFileSystem->Read(handle, &readBuffer[0], sizeof(char), buffSize);

	Document doc;
	doc.Parse(std::string(std::begin(readBuffer), std::end(readBuffer)));

	FromJson<T>(instance, doc);

	m_pFileSystem->CloseFile(handle);
	return true;
}

template<typename T>
inline bool JsonSerializer::ToJsonFile(T& instance, const std::string& filePath, bool bPretty)
{
	FH handle = m_pFileSystem->OpenFile(filePath.c_str(), FSOpenFlags::FSOF_WRITE);
	if (handle == INVALID_FH)
	{
		return false;
	}

	StringBuffer buffer(0, 65536);

	Document doc;
	ToJson(instance, doc);

	if (bPretty)
	{
		PrettyWriter<StringBuffer> writer(buffer);
		doc.Accept(writer);
	}
	else
	{
		Writer<StringBuffer> writer(buffer);
		doc.Accept(writer);
	}

	FSResult ret = m_pFileSystem->Write(handle, buffer.GetString(), sizeof(char), buffer.GetLength());
	if (ret == FSR_OK)
	{
		m_pFileSystem->CloseFile(handle);
		return true;
	}
	return false;
}
