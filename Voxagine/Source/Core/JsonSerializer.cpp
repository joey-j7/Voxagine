#include "pch.h"
#include "JsonSerializer.h"

#include "Core/ECS/World.h"
#include "Core/ECS/Entities/Camera.h"
#include "Core/ECS/Systems/Physics/PhysicsSystem.h"
#include "Core/Settings.h"
#include "Core/ECS/Systems/AudioSystem.h"
#include "Core/ECS/Components/VoxRenderer.h"
#include "Core/Resources/Formats/VoxModel.h"
#include "Core/ECS/Systems/Chunk/Chunk.h"
#include "Core/ECS/Systems/Chunk/ChunkSystem.h"
#include "Core/Threading/JobManager.h"
#include "Core/Application.h"
#include "Core/Objects/TSubclass.h"
#include "Core/System/FileSystem.h"
#include "ECS/Systems/AudioSystem.h"

JsonSerializer::JsonSerializer(Settings& settings, LoggingSystem& logger) :
	m_Settings(settings),
	m_Logger(logger)
{
}

void JsonSerializer::Initialize(FileSystem* pFileSystem)
{
	m_pFileSystem = pFileSystem;
}

bool JsonSerializer::SerializeWorld(World* pWorld, Document& worldDoc)
{
	Document::AllocatorType& worldAlloc = worldDoc.GetAllocator();

	worldDoc.SetObject();
	worldDoc.AddMember("Version", WORLD_VERSION, worldAlloc);

	Value world(kObjectType);
	world.AddMember("GroundTexture", pWorld->GetGroundTexturePath(), worldAlloc);

	/* Save physics settings */
	PhysicsSystem* physics = pWorld->GetSystem<PhysicsSystem>();
	Value physicsVal(kObjectType);
	physicsVal.AddMember("MaxParticles", physics->GetParticlePoolSize(), worldAlloc);
	physicsVal.AddMember("VoxelSize", physics->GetVoxelGrid()->GetVoxelSize(), worldAlloc);

	Value gridVal(kArrayType);
	uint32_t dimensionX, dimensionY, dimensionZ;
	physics->GetVoxelGrid()->GetDimensions(dimensionX, dimensionY, dimensionZ);
	gridVal.PushBack(dimensionX, worldAlloc);
	gridVal.PushBack(dimensionY, worldAlloc);
	gridVal.PushBack(dimensionZ, worldAlloc);
	physicsVal.AddMember("GridSize", gridVal, worldAlloc);

	world.AddMember("Physics", physicsVal, worldAlloc);

	Value chunkData(kObjectType);
	ChunkifyWorld(*pWorld, chunkData, UVector2(256, 256), worldAlloc);

	world.AddMember("ChunkData", chunkData, worldAlloc);
	worldDoc.AddMember("World", world, worldAlloc);
	return true;
}

void JsonSerializer::SerializeWorldAsync(World* pWorld, std::function<void(bool, Document&)> callback)
{
	//TODO: Run SerializeWorld via job and return callback
}

void JsonSerializer::SerializeWorldToFile(const std::string& filePath, World* pWorld, std::function<void(bool)> callback)
{
	//TODO: Run via job

	// Check if filepath is valid wld file
	if (filePath.substr(filePath.find_last_of(".") + 1) != m_Settings.GetWorldFileExtension())
	{
		callback(false);
		return;
	}

	FH handle = m_pFileSystem->OpenFile(filePath.c_str(), FSOpenFlags::FSOF_WRITE);
	if (handle == INVALID_FH)
	{
		callback(false);
		return;
	}

	StringBuffer buffer(0, 65536);
	Writer<StringBuffer> writer(buffer);

	Document doc;
	if (SerializeWorld(pWorld, doc))
	{
		doc.Accept(writer);
		m_pFileSystem->Write(handle, buffer.GetString(), sizeof(char), buffer.GetLength());
		callback(true);
		m_pFileSystem->CloseFile(handle);
		return;
	}

	m_pFileSystem->CloseFile(handle);
	callback(false);
}

bool JsonSerializer::DeserializeWorld(World& world, Document& worldDoc)
{
	if (worldDoc.HasMember("Version"))
	{
		double version = worldDoc["Version"].GetDouble();
		if (version >= 1.10)
		{
			Value& worldVal = worldDoc["World"];
			Value& physicsVal = worldVal["Physics"];
			Value& groundTextureVal = worldVal["GroundTexture"];
			Value& chunkGridSizeVal = worldVal["ChunkData"]["ChunkGrid"];
			Value& chunkSizeVal = worldVal["ChunkData"]["ChunkSize"];
			Value& voxelGridSizeVal = worldVal["Physics"]["GridSize"];
			Value& cameraChunkIndexVal = worldVal["ChunkData"]["CameraChunkIndex"];

			// Set ground texture
			world.m_GroundTexturePath = groundTextureVal.GetString();

			// Set physics
			uint32_t maxParticles = physicsVal["MaxParticles"].GetUint();
			uint32_t voxelSize = physicsVal["VoxelSize"].GetUint();

			//Calculate the minimum size of the voxel buffer grid
			UVector2 chunkGridSize(chunkGridSizeVal[0].GetUint(), chunkGridSizeVal[1].GetUint());
			UVector2 chunkSize(chunkSizeVal[0].GetUint(), chunkSizeVal[1].GetUint());
			UVector3 voxelGridSize(1 * chunkSize.x, voxelGridSizeVal[1].GetUint(), 1 * chunkSize.y);
			if (chunkGridSize.x > 1)
				voxelGridSize.x = 3 * chunkSize.x;
			if (chunkGridSize.y > 1)
				voxelGridSize.z = 3 * chunkSize.y;

			PhysicsSystem* pPhysicsSystem = new PhysicsSystem(&world, voxelGridSize, voxelSize, maxParticles, UVector3(chunkSize.x, voxelGridSize.y, chunkSize.y));
			world.SetPhysicsSystem(pPhysicsSystem);

			AudioSystem* pAudioSystem = new AudioSystem(&world);
			world.SetAudioSystem(pAudioSystem);

			UVector2 cameraChunkIndex(cameraChunkIndexVal[0].GetUint(), cameraChunkIndexVal[1].GetUint());
			std::unordered_map<uint32_t, Chunk*> chunks;
			chunks.reserve(chunkGridSize.x * chunkGridSize.y);

			// Set the entity id counter to count from the highest value
			// As an 64 bit integer it would practically never run out of unique id's this way
			uint64_t highestId = 0;
			Value& chunksVal = worldVal["ChunkData"]["Chunks"];
			for (SizeType i = 0; i < chunksVal.Size(); i++)
			{
				uint64_t chunkHighestId = GetHighestEntityID(chunksVal[i]["RootEntities"]);
				if (chunkHighestId > highestId)
					highestId = chunkHighestId;

				//Create the chunks for the chunk system
				UVector2 chunkId(floor(i / chunkGridSize.y), i % chunkGridSize.y);
				Chunk* pNewChunk = new Chunk(world.GetApplication(), &world, chunkId, UVector3(chunkSize.x, voxelGridSize.y, chunkSize.y), chunksVal[i]["RootEntities"]);
				if (chunkId == cameraChunkIndex)
				{
					pNewChunk->Load(UVector2(0, 0));
					pNewChunk->SetTargetLoaded(true);
				}
				chunks[chunkId.x + chunkId.y * chunkGridSize.x] = pNewChunk;
			}
			
			if (highestId + 1 > Entity::EntityIdCounter)
				Entity::EntityIdCounter = highestId;

			ChunkSystem* pChunkSystem = new ChunkSystem(&world, chunks, chunkSize, chunkGridSize * chunkSize);
			world.SetChunkSystem(pChunkSystem);

			return true;
		}
		else if (version <= 1.02)
		{
			Value& worldVal = worldDoc["World"];
			Value& physicsVal = worldVal["Physics"];
			Value& rootEntitiesVal = worldVal["RootEntities"];
			Value& groundTextureVal = worldVal["GroundTexture"];

			// Set ground texture
			world.m_GroundTexturePath = groundTextureVal.GetString();

			// Set physics
			uint32_t maxParticles = physicsVal["MaxParticles"].GetUint();
			uint32_t voxelSize = physicsVal["VoxelSize"].GetUint();
			Vector3 gridSize(physicsVal["GridSize"][0].GetFloat(), physicsVal["GridSize"][1].GetFloat(), physicsVal["GridSize"][2].GetFloat());

			PhysicsSystem* pPhysicsSystem = new PhysicsSystem(&world, gridSize, voxelSize, maxParticles);
			world.SetPhysicsSystem(pPhysicsSystem);

			AudioSystem* pAudioSystem = new AudioSystem(&world);
			world.SetAudioSystem(pAudioSystem);

			// Set the entity id counter to count from the highest value
			// As an 64 bit integer it would practically never run out of unique id's this way
			uint64_t highestId = GetHighestEntityID(rootEntitiesVal) + 1;
			if (highestId > Entity::EntityIdCounter)
				Entity::EntityIdCounter = highestId;

			// Add all entities to world
			for (SizeType i = 0; i < rootEntitiesVal.Size(); i++)
			{
				Entity* pEntity = ValueToEntity(rootEntitiesVal[i], world);
				AddRootEntityToWorld(world, pEntity);
			}

			//Added to support saving to 1.10 .wld version
			world.SetChunkSystem(new ChunkSystem(&world, std::unordered_map<uint32_t, Chunk*>(), UVector2(256,256), UVector2(gridSize.x, gridSize.z)));

			return true;
		}
		return false;
	}
	return false;
}

bool JsonSerializer::DeserializeWorldFromFile(World& world, const std::string& filePath)
{
	// Check if filepath is valid wld file
	if (filePath.substr(filePath.find_last_of(".") + 1) != m_Settings.GetWorldFileExtension())
		return false;

	FH handle = m_pFileSystem->OpenFile(filePath.c_str(), FSOpenFlags::FSOF_READ);
	if (handle == INVALID_FH)
	{
		return false;
	}

	FSize buffSize = m_pFileSystem->GetFileSize(handle);
	std::vector<char> readBuffer;
	readBuffer.resize(buffSize);
	m_pFileSystem->Read(handle, &readBuffer[0], sizeof(char), buffSize);

	Document worldDoc;
	worldDoc.Parse(std::string(std::begin(readBuffer), std::end(readBuffer)));

	if (DeserializeWorld(world, worldDoc))
	{
		m_pFileSystem->CloseFile(handle);
		return true;
	}

	m_pFileSystem->CloseFile(handle);

	return false;
}

bool JsonSerializer::SerializeEntityToFile(const std::string& filePath, Entity* pEntity)
{
	// Check if filepath is valid wld file
	if (filePath.substr(filePath.find_last_of(".") + 1) != m_Settings.GetPrefabFileExtension())
	{
		return false;
	}

	FH handle = m_pFileSystem->OpenFile(filePath.c_str(), FSOpenFlags::FSOF_WRITE);
	if (handle == INVALID_FH)
	{
		return false;
	}

	StringBuffer buffer(0, 65536);
	Writer<StringBuffer> writer(buffer);

	Document doc;
	doc.SetObject();
	Value entityVal(kObjectType);

	EntityToValue(pEntity, entityVal, doc.GetAllocator());
	doc.AddMember("Prefab", entityVal, doc.GetAllocator());
	doc.Accept(writer);

	m_pFileSystem->Write(handle, buffer.GetString(), sizeof(char), buffer.GetLength());
	m_pFileSystem->CloseFile(handle);
	return true;
}

Entity* JsonSerializer::DeserializeEntityFromFile(const std::string& filePath, World& world)
{
	// Check if filepath is valid wld file
	if (filePath.substr(filePath.find_last_of(".") + 1) != m_Settings.GetPrefabFileExtension())
		return false;

	FH handle = m_pFileSystem->OpenFile(filePath.c_str(), FSOpenFlags::FSOF_READ);
	if (handle == INVALID_FH)
	{
		return false;
	}

	FSize buffSize = m_pFileSystem->GetFileSize(handle);
	std::vector<char> readBuffer;
	readBuffer.resize(buffSize);
	m_pFileSystem->Read(handle, &readBuffer[0], sizeof(char), buffSize);

	Document worldDoc;
	worldDoc.Parse(std::string(std::begin(readBuffer), std::end(readBuffer)));

	m_pFileSystem->CloseFile(handle);
	if (worldDoc.HasMember("Prefab"))
	{
		Entity* pEntity = ValueToEntity(worldDoc["Prefab"], world, true);
		if (pEntity)
		{
			AddRootEntityToWorld(world, pEntity);
			return pEntity;
		}
	}
	return nullptr;
}

Entity* JsonSerializer::ValueToEntity(Value& val, World& world, bool bGenerateNewId, Entity* pParent)
{
	// Get Entity type from string
	std::string entityTypeName = val["EntityType"].GetString();
	rttr::type entityType = rttr::type::get_by_name(entityTypeName);

	// Check if the type is valid before creation
	if (!entityType.is_valid())
		return nullptr;

	// Create entity using rttr type
	rttr::variant entityVar = entityType.create({ &world });
	Entity* pEntity = entityVar.get_value<Entity*>();

	// Set the parent before initializing the components
	if (pParent != nullptr)
		pEntity->SetParent(pParent);

	// Set all entity properties
	rttr::instance instance = *pEntity;
	for (rttr::property prop : entityType.get_properties())
	{
		if (prop.get_name().to_string() == "ID" && bGenerateNewId)
		{
			if (val.HasMember(prop.get_name().to_string()))
				m_vOldPrefabIDs.insert({ val[prop.get_name().to_string()].GetInt64(), pEntity->GetId() });
			continue;
		}

		if (val.HasMember(prop.get_name().to_string()))
		{
			SetPropertyFromValue(&world, val[prop.get_name().to_string()], prop, instance);
		}
	}

	// Create and add all components to entity
	Value& componentsVal = val["Components"];
	for (SizeType i = 0; i < componentsVal.Size(); i++)
	{
		std::string compTypeName = componentsVal[i]["ComponentType"].GetString();
		if (compTypeName != "Transform")
		{
			Component* pComponent = ValueToComponent(world, componentsVal[i], pEntity);
			pEntity->AddComponent(pComponent);
		}
		else
		{
			// Special case for transform since its already added in the entity constructor
			rttr::instance transformInstance = *pEntity->GetTransform();
			for (rttr::property prop : rttr::type::get<Transform>().get_properties())
			{
				if (componentsVal[i].HasMember(prop.get_name().to_string()))
				{
					SetPropertyFromValue(&world, componentsVal[i][prop.get_name().to_string()], prop, transformInstance);
				}
			}
		}
	}

	// Create and add all entity children to parent entity
	Value& childrenVal = val["Children"];
	for (SizeType i = 0; i < childrenVal.Size(); i++)
	{
		Entity* pChild = ValueToEntity(childrenVal[i], world, bGenerateNewId, pEntity);
	}
	return pEntity;
}

Component* JsonSerializer::ValueToComponent(World& world, Value& val, Entity* pEntity)
{
	// Get Component type from string
	std::string componentTypeName = val["ComponentType"].GetString();
	rttr::type componentType = rttr::type::get_by_name(componentTypeName);

	// Check if the type is valid before creation
	if (!componentType.is_valid())
		return nullptr;

	// Create Component using rttr type
	rttr::variant componentVar = componentType.create({ pEntity });
	Component* pComponent = componentVar.get_value<Component*>();

	// Set all component properties
	rttr::instance instance(componentVar);
	for (rttr::property prop : componentType.get_properties())
	{
		if (val.HasMember(prop.get_name().to_string()))
		{
			SetPropertyFromValue(&world, val[prop.get_name().to_string()], prop, instance);
		}
	}
	return pComponent;
}

void JsonSerializer::ComponentToValue(Component* pComponent, Value& val, Document::AllocatorType& alloc)
{
	rttr::instance compInstance = *pComponent;
	rttr::type compType = rttr::type::get(*pComponent);
	val.AddMember("ComponentType", Value(compType.get_name().to_string().c_str(), alloc), alloc);

	for (rttr::property prop : compType.get_properties())
		AddPropertyAsMember(val, prop, compInstance, alloc);
}

void JsonSerializer::EntityToValue(Entity* pEntity, Value& val, Document::AllocatorType& alloc)
{
	rttr::instance instance = *pEntity;
	rttr::type entityType = rttr::type::get(*pEntity);
	val.AddMember("EntityType", Value(entityType.get_name().to_string().c_str(), alloc), alloc);

	for (rttr::property prop : entityType.get_properties())
		AddPropertyAsMember(val, prop, instance, alloc);

	Value components(kArrayType);
	const std::vector<Component*>& entityComponents = pEntity->GetComponents();
	for (Component* pComponent : entityComponents)
	{
		Value componentVal(kObjectType);
		ComponentToValue(pComponent, componentVal, alloc);
		components.PushBack(componentVal, alloc);
	}

	val.AddMember("Components", components, alloc);

	/* Recursive loop through all child entities */
	Value children(kArrayType);
	const std::vector<Entity*>& childEntities = pEntity->GetChildren();
	for (Entity* child : childEntities)
	{
		Value childVal(kObjectType);
		EntityToValue(child, childVal, alloc);
		children.PushBack(childVal, alloc);
	}

	val.AddMember("Children", children, alloc);
}

void JsonSerializer::AddPropertyAsMember(Value& val, rttr::property& prop, rttr::instance& instance, Document::AllocatorType& alloc)
{
	Value name(prop.get_name().to_string().c_str(), alloc);
	Value propVal;
	VariantToValue(propVal, prop.get_value(instance), alloc);
	val.AddMember(name, propVal, alloc);
}

void JsonSerializer::SetPropertyFromValue(World* world, const Value& val, rttr::property& prop, rttr::instance& instance)
{
	rttr::variant var;
	rttr::variant propVar = prop.get_value(instance);

	if (prop.get_type().is_associative_container() || prop.get_type().is_sequential_container())
		ContainerValueToVariant(world, instance, var, val, prop, prop.get_type(), propVar);
	else ValueToVariant(world, instance, var, val, prop, prop.get_type());

	if(!prop.set_value(instance, var))
	{
		// TODO error message when not able to set the message.	
	}
}

void JsonSerializer::ChunkifyWorld(World& world, Value& chunkDataVal, UVector2 chunkSize, Document::AllocatorType& alloc)
{
	UVector3 voxelGridDimensions = world.GetVoxelGrid()->GetDimensions();
	UVector2 dimensions = world.GetWorldSize();
	uint32_t numX = dimensions.x / chunkSize.x;
	uint32_t numY = dimensions.y / chunkSize.y;

	Value chunkArr(kArrayType);

	Value gridVal(kArrayType);
	gridVal.PushBack(numX, alloc);
	gridVal.PushBack(numY, alloc);
	chunkDataVal.AddMember("ChunkGrid", gridVal, alloc);

	Value chunkSizeVal(kArrayType);
	chunkSizeVal.PushBack(chunkSize.x, alloc);
	chunkSizeVal.PushBack(chunkSize.y, alloc);
	chunkDataVal.AddMember("ChunkSize", chunkSizeVal, alloc);

	std::unordered_map<uint32_t, Value> chunkRootEntities;
	for (uint32_t x = 0; x < numX; ++x)
	{
		for (uint32_t y = 0; y < numY; ++y)
		{
			chunkRootEntities[x + y * numX] = Value(kArrayType);
		}
	}

	//Get camera chunk position for initial chunk load
	Value cameraChunkIndexVal(kArrayType);
	cameraChunkIndexVal.PushBack(0, alloc);
	cameraChunkIndexVal.PushBack(0, alloc);

	//Save all entities inside the chunks to the document
	const std::unordered_map<uint32_t, Chunk*>& chunks = world.GetChunkSystem()->GetChunks();
	for (auto& chunk : chunks)
	{
		if (!chunk.second->IsLoaded())
		{
			const std::vector<Value>& savedEntities = chunk.second->GetRootEntities();
			for (SizeType i = 0; i < savedEntities.size(); i++)
			{
				//find camera from JSON value
				if (savedEntities[i].HasMember("EntityType") && savedEntities[i]["EntityType"] == "Camera")
				{
					cameraChunkIndexVal[0] = chunk.second->GetChunkIndex().x;
					cameraChunkIndexVal[1] = chunk.second->GetChunkIndex().y;
				}

				Value copy;
				copy.CopyFrom(savedEntities[i], alloc);
				chunkRootEntities[chunk.first].PushBack(copy, alloc);
			}
		}
		else
		{
			const std::vector<Entity*>& entities = world.GetEntities();
			std::vector<std::pair<Entity*, bool>> foundEntities;
			chunk.second->FindEntitiesInChunk(entities, foundEntities);
			for (std::pair<Entity*, bool>& entityPair : foundEntities)
			{
				if (entityPair.first->get_type() == rttr::type::get<Camera>())
				{
					cameraChunkIndexVal[0] = chunk.second->GetChunkIndex().x;
					cameraChunkIndexVal[1] = chunk.second->GetChunkIndex().y;
				}

				Value entityVal(kObjectType);
				EntityToValue(entityPair.first, entityVal, alloc);
				chunkRootEntities[chunk.first].PushBack(entityVal, alloc);
			}
		}
	}
	chunkDataVal.AddMember("CameraChunkIndex", cameraChunkIndexVal, alloc);

	for (uint32_t x = 0; x < numX; ++x)
	{
		for (uint32_t y = 0; y < numY; ++y)
		{
			Value chunk(kObjectType);
			chunk.AddMember("RootEntities", chunkRootEntities[x + y * numX], alloc);
			chunkArr.PushBack(chunk, alloc);
		}
	}

	chunkDataVal.AddMember("Chunks", chunkArr, alloc);
}

void JsonSerializer::VariantToValue(Value& val, const rttr::variant& variant, Document::AllocatorType& alloc)
{
	if (variant.get_type() == rttr::type::get<bool>())
		val = Value(variant.to_bool());
	else if (variant.get_type() == rttr::type::get<std::string>())
	{
		val = Value(variant.to_string().c_str(), alloc);
		// can grab the string two ways
		const auto& StringType = rttr::type::get<std::string>();
		std::string NewValue = variant.can_convert(StringType) ? variant.get_value<std::string>() : variant.to_string();
		val = Value(NewValue.c_str(), alloc);
	}
	else if (variant.get_type() == rttr::type::get<float>())
		val = Value(variant.to_float());
	else if (variant.get_type() == rttr::type::get<double>())
		val = Value(variant.to_double());
	else if (variant.get_type() == rttr::type::get<uint16_t>())
		val = Value(variant.to_int16());
	else if (variant.get_type() == rttr::type::get<uint32_t>())
		val = Value(variant.to_uint32());
	else if (variant.get_type() == rttr::type::get<uint64_t>())
		val = Value(variant.to_uint64());
	else if (variant.get_type() == rttr::type::get<int16_t>())
		val = Value(variant.to_int16());
	else if (variant.get_type() == rttr::type::get<int32_t>())
		val = Value(variant.to_int32());
	else if (variant.get_type() == rttr::type::get<int64_t>())
		val = Value(variant.to_int64());
	else if (variant.get_type() == rttr::type::get<Vector3>())
	{
		val = Value(kArrayType);
		Vector3 tempVec = variant.convert<Vector3>();

		val.PushBack(tempVec.x, alloc);
		val.PushBack(tempVec.y, alloc);
		val.PushBack(tempVec.z, alloc);
	}
	else if (variant.get_type() == rttr::type::get<UVector3>())
	{
		val = Value(kArrayType);
		UVector3 tempVec = variant.convert<UVector3>();

		val.PushBack(tempVec.x, alloc);
		val.PushBack(tempVec.y, alloc);
		val.PushBack(tempVec.z, alloc);
	}
	else if (variant.get_type() == rttr::type::get<Vector2>())
	{
		val = Value(kArrayType);
		Vector2 tempVec = variant.convert<Vector2>();

		val.PushBack(tempVec.x, alloc);
		val.PushBack(tempVec.y, alloc);
	}
	else if (variant.get_type() == rttr::type::get<UVector2>())
	{
		val = Value(kArrayType);
		UVector2 tempVec = variant.convert<UVector2>();

		val.PushBack(tempVec.x, alloc);
		val.PushBack(tempVec.y, alloc);
	}
	else if (variant.get_type() == rttr::type::get<VColor>())
	{
		const VColor tempColor = variant.convert<VColor>();
		val = Value(tempColor.inst.Color);
	}
	else if (variant.get_type() == rttr::type::get<PlayerPrefs::Pair*>())
	{
		const auto pair = variant.get_value<PlayerPrefs::Pair*>();
		if (pair)
		{
			if (const auto & int_pair = dynamic_cast<PlayerPrefs::IntPlayerPair*>(pair))
			{
				return VariantToValue(val, int_pair->value, alloc);
			}

			if (const auto & float_pair = dynamic_cast<PlayerPrefs::FloatPlayerPair*>(pair))
			{
				return VariantToValue(val, float_pair->value, alloc);
			}

			if (const auto & string_pair = dynamic_cast<PlayerPrefs::StringPlayerPair*>(pair))
			{
				return VariantToValue(val, string_pair->value, alloc);
			}
		}

		val = Value(kObjectType);
	}
	else if(Utils::CheckDerivedType(variant.get_type(), rttr::type::get<Entity*>()))
	{
		const auto pEntity = variant.get_value<Entity*>();
		// -1 mean nullptr
		val = Value((pEntity) ? static_cast<int64_t>(pEntity->GetId()) : int64_t(-1));
	}
	else if (Utils::CheckDerivedType(variant.get_type(), rttr::type::get<Component*>()))
	{
		const auto pComponent = variant.get_value<Component*>();
		// -1 mean nullptr
		val = Value((pComponent) ? static_cast<int64_t>(pComponent->GetOwner()->GetId()) : int64_t(-1));
	}
	// else if (Utils::CheckDerivedType(variant.get_type(), rttr::type::get<VObject*>()))
	// {
	// 	val = Value(kObjectType);
	// 	rttr::instance objInstance = variant;
	// 	val.AddMember("Type", Value(objInstance.get_derived_type().get_name().to_string().c_str(), alloc), alloc);
	//
	// 	Value dataVal(kObjectType);
	// 	for (rttr::property objProp : objInstance.get_derived_type().get_properties())
	// 	{
	// 		Value name(objProp.get_name().to_string().c_str(), alloc);
	// 		Value memberVal;
	// 		VariantToValue(memberVal, objProp.get_value(objInstance), alloc);
	// 		dataVal.AddMember(name, memberVal, alloc);
	// 	}
	//
	// 	val.AddMember("Data", dataVal, alloc);
	// }
	else if (Utils::CheckDerivedType(variant.get_type(), rttr::type::get<VClass>()))
	{
		val = Value(kObjectType);
		rttr::instance objInstance = variant;
		for (rttr::property objProp : variant.get_type().get_properties())
		{
			Value name(objProp.get_name().to_string().c_str(), alloc);
			Value memberVal;
			VariantToValue(memberVal, objProp.get_value(objInstance), alloc);
			val.AddMember(name, memberVal, alloc);
		}
	}
	else if (variant.get_type().is_enumeration())
	{
		rttr::enumeration enumType = variant.get_type().get_enumeration();
		val = Value(enumType.value_to_name(variant).to_string().c_str(), alloc);
	}
	else if (variant.get_type().is_associative_container())
	{
		val = Value(kObjectType);
		rttr::variant_associative_view arrView = variant.create_associative_view();

		if (!arrView.get_key_type().is_arithmetic() && arrView.get_key_type() != rttr::type::get<std::string>())
		{
			m_Logger.Log(LOGLEVEL_ERROR, "JsonSerializer", "Invalid key type for map, key type must be arithmetic or string");
			return;
		}

		for (auto& item : arrView)
		{
			Value keyVal, valueVal;
			VariantToValue(keyVal, item.first.extract_wrapped_value(), alloc);
			VariantToValue(valueVal, item.second.extract_wrapped_value(), alloc);

			if (keyVal.IsInt())
				keyVal = Value(std::to_string(keyVal.GetInt()), alloc);
			else if (keyVal.IsFloat())
				keyVal = Value(std::to_string(keyVal.GetFloat()), alloc);
			else if (keyVal.IsDouble())
				keyVal = Value(std::to_string(keyVal.GetDouble()), alloc);
			else if (keyVal.IsInt64())
				keyVal = Value(std::to_string(keyVal.GetInt64()), alloc);
			else if (keyVal.IsUint())
				keyVal = Value(std::to_string(keyVal.GetUint()), alloc);
			else if (keyVal.IsUint())
				keyVal = Value(std::to_string(keyVal.GetUint64()), alloc);

			val.AddMember(keyVal, valueVal, alloc);
		}
	}
	else if (variant.get_type().is_sequential_container())
	{
		val = Value(kArrayType);
		rttr::variant_sequential_view arrView = variant.create_sequential_view();
		for (const rttr::variant& item : arrView)
		{
			Value arrVal;
			VariantToValue(arrVal, item.extract_wrapped_value(), alloc);
			val.PushBack(arrVal, alloc);
		}
	}
	else if (variant.get_type().is_class())
	{
		val = Value(kObjectType);
		rttr::instance objInstance(variant);
		for (rttr::property objProp : variant.get_type().get_properties())
		{
			Value name(objProp.get_name().to_string().c_str(), alloc);
			Value memberVal;
			VariantToValue(memberVal, objProp.get_value(objInstance), alloc);
			val.AddMember(name, memberVal, alloc);
		}
	}
}

void JsonSerializer::ValueToVariant(World* world, rttr::instance& instance, rttr::variant& variant, const Value& val, rttr::property& property, const rttr::type& variantType)
{
	if (variantType == rttr::type::get<bool>() && val.IsBool())
		variant = val.GetBool();
	else if (variantType == rttr::type::get<std::string>() && val.IsString())
		variant = std::string(val.GetString());
	else if (variantType == rttr::type::get<float>() && val.IsFloat())
		variant = val.GetFloat();
	else if (variantType == rttr::type::get<double>() && val.IsDouble())
		variant = val.GetDouble();
	else if ((variantType == rttr::type::get<uint16_t>() || variantType == rttr::type::get<uint32_t>()) && val.IsUint())
		variant = val.GetUint();
	else if (variantType == rttr::type::get<uint64_t>() && val.IsUint64())
		variant = val.GetUint64();
	else if ((variantType == rttr::type::get<int16_t>() || variantType == rttr::type::get<int32_t>()) && val.IsInt())
		variant = val.GetInt();
	else if (variantType == rttr::type::get<int64_t>() && val.IsInt64())
		variant = val.GetInt64();
	else if (variantType == rttr::type::get<Vector3>() && val.IsArray())
		variant = Vector3(val[0].GetFloat(), val[1].GetFloat(), val[2].GetFloat());
	else if (variantType == rttr::type::get<UVector3>() && val.IsArray())
		variant = UVector3(val[0].GetInt(), val[1].GetInt(), val[2].GetInt());
	else if (variantType == rttr::type::get<Vector2>() && val.IsArray())
		variant = Vector2(val[0].GetFloat(), val[1].GetFloat());
	else if (variantType == rttr::type::get<UVector2>() && val.IsArray())
		variant = UVector2(val[0].GetInt(), val[1].GetInt());
	else if (variantType == rttr::type::get<VColor>() && val.IsUint())
		variant = VColor(val.GetUint());
	else if (variantType == rttr::type::get<PlayerPrefs::Pair*>())
	{
		const auto& it = val;
		if (it.IsString())
		{
			const auto string_pair = new PlayerPrefs::StringPlayerPair();
			string_pair->value = it.GetString();

			PlayerPrefs::Pair* insert_pair = string_pair;
			variant = insert_pair;
		}

		if (it.IsInt())
		{
			const auto int_pair = new PlayerPrefs::IntPlayerPair();
			int_pair->value = it.GetInt();

			PlayerPrefs::Pair* insert_pair = int_pair;

			variant = insert_pair;
		}

		if (it.IsFloat())
		{
			const auto float_pair = new PlayerPrefs::FloatPlayerPair();
			float_pair->value = it.GetFloat();

			PlayerPrefs::Pair* insert_pair = float_pair;

			variant = insert_pair;
		}

	}
	else if ((variantType == rttr::type::get<Entity*>() || Utils::CheckDerivedType(variantType, rttr::type::get<Entity*>())) && val.IsInt64())
	{
		if (!rttr::type::get<VClass>().is_base_of(instance.get_derived_type()) && world)
			world->m_vWorldConnections.push_back({ instance, property, variantType, val.GetInt64() });
	}
	else if ((variantType == rttr::type::get<Component*>() || Utils::CheckDerivedType(variantType, rttr::type::get<Component*>())) && val.IsInt64())
	{
		if (!rttr::type::get<VClass>().is_base_of(instance.get_derived_type()) && world)
			world->m_vWorldConnections.push_back({ instance, property, variantType, val.GetInt64() });
	}
	// I don't have to create this because it is a stack member
// {
// 	if (!val.HasMember("Data") && !val.HasMember("Type"))
// 	{
// 		m_Logger.Log(LOGLEVEL_ERROR, "JsonSerializer", "Failed parse document to VObject");
// 		return;
// 	}
//
// 	StringBuffer buffer;
// 	Writer<StringBuffer> writer(buffer);
// 	val.Accept(writer);
//
// 	// const Value& classVal = val["Data"];
// 	std::string className = val["Type"].GetString();
// 	rttr::type classType = rttr::type::get_by_name(className);
// 	variant = classType.create();
// 	rttr::instance rTest = variant;
//
// 	Document doc;
// 	doc.Parse(buffer.GetString());
// 	FromJson(rTest, doc);
// }
// I don't have to create this because it is a stack member
	else if (Utils::CheckDerivedType(variantType, rttr::type::get<VClass>()))
	{
		variant = property.get_value(instance);
		rttr::instance objInstance = variant;
		for (rttr::property objProp : objInstance.get_type().get_properties())
		{
			std::string member = objProp.get_name().to_string();

			if (val.HasMember(member))
			{
				SetPropertyFromValue(world, val[objProp.get_name().to_string()], objProp, objInstance);
			}
		}
	}
	else if (variantType.is_enumeration())
	{
		rttr::enumeration enumType = variantType.get_enumeration();
		variant = enumType.name_to_value(val.GetString());
	}
	else if (variantType.is_class() && val.IsObject())
	{
		rttr::variant objVar = variantType.create();
		rttr::instance objInstance(objVar);

		for (rttr::property objProp : variantType.get_properties())
		{
			if (val.HasMember(objProp.get_name().to_string()))
			{
				SetPropertyFromValue(world, val[objProp.get_name().to_string()], objProp, objInstance);
			}
		}
		
		variant.swap(objVar);
	}
}

void JsonSerializer::ValueToVariant(World* world, rttr::instance& instance, rttr::variant& variant, const Value& val, rttr::property& property, const rttr::type& variantType, const int& index)
{
	// if it a array property that we need the index of the element.
	if ((Utils::CheckDerivedType(variantType, rttr::type::get<Entity*>())) && val.IsInt64())
	{
		if (rttr::type::get<VClass>().is_base_of(instance.get_derived_type()))
			return;

		Entity* pEntity = instance.try_convert<Entity>();
		Component* pComponent = instance.try_convert<Component>();

		if(world)
			world->m_vWorldConnections.push_back({ instance, property, variantType, val.GetInt64(), index });
	}
	// Same goes for the component array types
	else if ((Utils::CheckDerivedType(variantType, rttr::type::get<Component*>())) && val.IsInt64())
	{
		if (rttr::type::get<VClass>().is_base_of(instance.get_derived_type()))
			return;

		Entity* pEntity = instance.try_convert<Entity>();
		Component* pComponent = instance.try_convert<Component>();

		if(world)
			world->m_vWorldConnections.push_back({ instance, property, variantType, val.GetInt64(), index });
	}
	else if (Utils::CheckDerivedType(variantType, rttr::type::get<VClass>()) && property.get_type().is_sequential_container())
	{
		variant = variantType.create();
		rttr::instance objInstance = variant;
		for (rttr::property objProp : objInstance.get_type().get_properties())
		{
			std::string member = objProp.get_name().to_string();

			if (val.HasMember(member))
			{
				SetPropertyFromValue(world, val[objProp.get_name().to_string()], objProp, objInstance);
			}
		}

	}
	// if we have a normal supported array then it does not matter what the index is do normal stuff within the function.
	else
	{
		ValueToVariant(world, instance, variant, val, property, variantType);
	}
}

void JsonSerializer::ContainerValueToVariant(World* world, rttr::instance& instance, rttr::variant& variant, const Value& val, rttr::property& property, const rttr::type& variantType, rttr::variant& containerVar)
{
	if (variantType.is_associative_container() && val.IsObject())
	{
		rttr::variant var = containerVar;
		rttr::variant_associative_view arrView = var.create_associative_view();

		for (Value::ConstMemberIterator iter = val.MemberBegin(); iter != val.MemberEnd(); ++iter)
		{
			rttr::variant key, value;

			// Get key variant value
			if (arrView.get_key_type().is_arithmetic())
			{
				if (arrView.get_key_type() == rttr::type::get<int16_t>() || arrView.get_key_type() == rttr::type::get<int32_t>())
					key = std::stoi(iter->name.GetString());
				else if (arrView.get_key_type() == rttr::type::get<int64_t>())
					key = std::stoll(iter->name.GetString());
				else if (arrView.get_key_type() == rttr::type::get<uint16_t>() || arrView.get_key_type() == rttr::type::get<uint32_t>())
					key = std::stoul(iter->name.GetString());
				else if (arrView.get_key_type() == rttr::type::get<uint64_t>())
					key = std::stoull(iter->name.GetString());
				else if (arrView.get_key_type() == rttr::type::get<float>())
					key = std::stof(iter->name.GetString());
				else if (arrView.get_key_type() == rttr::type::get<double>())
					key = std::stod(iter->name.GetString());
			}
			else ValueToVariant(world, instance, key, iter->name, property, arrView.get_key_type());

			// Get value
			if (arrView.get_value_type().is_associative_container() || arrView.get_value_type().is_sequential_container())
			{
				m_Logger.Log(LOGLEVEL_ERROR, "JsonSerializer", "Associative containers cannot have containers as their value. See type: " + variantType.get_name().to_string());
				return;
			}
			
			ValueToVariant(world, instance, value, iter->value, property, arrView.get_value_type());

			// Try insert key value pair
			value.convert(arrView.get_value_type());
			const auto insertResult = arrView.insert(key, value);
			if (!insertResult.second)
			{
				m_Logger.Log(LOGLEVEL_ERROR, "JsonSerializer", "Failed to set associative container item. See type: " + variantType.get_name().to_string());
			}
		}

		variant.swap(var);
	}
	else if (variantType.is_sequential_container() && val.IsArray())
	{
		const rttr::variant var = containerVar;
		rttr::variant_sequential_view arrView = var.create_sequential_view();
	
		if(arrView.set_size(val.Size()))
		{
			// TODO we need to check why we can't change the size even though it is dynamic.
			// m_Logger.Log(LOGLEVEL_ERROR, "JsonSerializer", "Not able to set size of container " + variantType.get_name().to_string());
		}

		if (val.Size() > 0)
		{
			const bool isContainer = (arrView.get_value(0).extract_wrapped_value().is_associative_container() || 
								arrView.get_value(0).extract_wrapped_value().is_sequential_container());

			for (SizeType i = 0; i < val.Size(); i++)
			{

				rttr::variant arrVar;

				if (isContainer) 
				{
					arrVar = arrView.get_value(i).extract_wrapped_value();
					ContainerValueToVariant(world, instance, arrVar, val[i], property, arrView.get_value_type(), arrVar);
				}
				else ValueToVariant(world, instance, arrVar, val[i], property, arrView.get_value_type(), static_cast<int>(i));		

				if (arrVar.get_type().is_wrapper())
				{
					m_Logger.Log(LOGLEVEL_ERROR, "JsonSerializer", "Wrapped container items aren't supported, make sure policy::ctor::as_object is set on container items. See type: " + arrVar.get_type().get_name().to_string());
				}
				else if (arrVar.convert(arrView.get_value_type()) && !arrView.set_value(i, arrVar))
				{
					m_Logger.Log(LOGLEVEL_ERROR, "JsonSerializer", "Failed to set sequential container item. See type: " + variantType.get_name().to_string());
				}
			}
		}
		variant = var;
	}
}

uint64_t JsonSerializer::GetHighestEntityID(Value& rootEntityVal)
{
	uint64_t id = 0;
	for (SizeType i = 0; i < rootEntityVal.Size(); i++)
	{
		if (rootEntityVal[i].HasMember("ID"))
		{
			uint64_t entityId = rootEntityVal[i]["ID"].GetUint64();
			if (entityId > id)
				id = entityId;
		}

		uint64_t highestChildId = GetHighestEntityID(rootEntityVal[i]["Children"]);
		if (highestChildId > id)
			id = highestChildId;
	}

	return id;
}

void JsonSerializer::AddRootEntityToWorld(World& world, Entity* pEntity)
{
	if (pEntity->get_type().is_derived_from(rttr::type::get<Camera>()))
	{
		Camera* pCamera = static_cast<Camera*>(pEntity);
		if (pCamera->IsMainCamera() && world.GetMainCamera() == nullptr)
			world.SetMainCamera(pCamera);
	}

	world.AddEntity(pEntity);
	for (Entity* pChild : pEntity->GetChildren())
		AddRootEntityToWorld(world, pChild);
}

void JsonSerializer::ToJson(rttr::instance instance, Document & doc)
{
	rttr::type classType = instance.get_derived_type();

	doc.SetObject();
	doc.AddMember("Type", Value(classType.get_name().to_string().c_str(), doc.GetAllocator()), doc.GetAllocator());

	Value dataVal(kObjectType);

	for (rttr::property prop : classType.get_properties())
	{
		AddPropertyAsMember(dataVal, prop, instance, doc.GetAllocator());
	}

	doc.AddMember("Data", dataVal, doc.GetAllocator());
}

void JsonSerializer::ResolveWorldLinks(World& pWorld)
{
	if (pWorld.m_vWorldConnections.empty())
		return;

	for (const auto& connection : pWorld.m_vWorldConnections)
	{
		// if we have -1 that means it has not been set yet.
		if (connection.iEntityId == -1)
			continue;

		auto number = connection.iEntityId;
		const auto fIter = m_vOldPrefabIDs.find(number);

		// See if we have the same number or else find a different entity
		if (fIter != m_vOldPrefabIDs.end())
			number = (connection.iEntityId != fIter->second) ? fIter->second : connection.iEntityId;

		// find the entity that needs to be connected
		Entity* pVarEntity = pWorld.FindEntity(number);
		if (pVarEntity == nullptr)
			continue;

		rttr::variant varEntity = pVarEntity;
		if (!varEntity.is_valid()) continue;

		// if we are dealing with a component that has a property that needs to be linked.
		if(auto pComponent = connection.rInstance.try_convert<Component>())
		{
			// search the entity where a property needs to be connected
			if(auto pSearchEntity = pWorld.FindEntity(pComponent->GetOwner()->GetId()))
			{
				// grab the current component of the search entity
				Component* pCurrentComponent = pSearchEntity->GetComponent(pComponent->get_type());

				// if we are dealing with a array inside a component
				if(connection.rProperty.get_type().is_sequential_container())
				{
					// Fill the container with the right information
					rttr::instance instComponent = pCurrentComponent;

					if (!SetInstanceArrayProperty(instComponent, connection.rProperty, varEntity, connection.rType, connection.iIndex))
					{
						m_Logger.Log(LOGLEVEL_ERROR, "JsonSerializer", "Could not insert entity or component");
					}
				}
				else
				{
					rttr::instance instComponent = pCurrentComponent;
					if (!SetInstanceProperty(instComponent, connection.rProperty, varEntity, connection.rType))
					{
						m_Logger.Log(LOGLEVEL_ERROR, "JsonSerializer", "Could not connect entity or component");
					}
				}
			}
		}
		// if we are dealing with an entity that ha a property that needs to be linked.
		else if (const auto & pEntity = connection.rInstance.try_convert<Entity>())
		{
			if (auto pSearchEntity = pWorld.FindEntity(pEntity->GetId()))
			{
				if (connection.rProperty.get_type().is_sequential_container())
				{
					// Fill the container with the right information
					rttr::instance instEntity = pSearchEntity;

					// Create an array view
					if (!SetInstanceArrayProperty(instEntity, connection.rProperty, varEntity, connection.rType, connection.iIndex))
					{
						m_Logger.Log(LOGLEVEL_ERROR, "JsonSerializer", "Could not insert entity or component");
					}
				}
				// normal property
				else
				{
					rttr::instance instEntity = pSearchEntity;
					if (!SetInstanceProperty(instEntity, connection.rProperty, varEntity, connection.rType))
					{
						m_Logger.Log(LOGLEVEL_ERROR, "JsonSerializer", "Could not connect entity or component");
					}
				}
			}
		}
	}

	pWorld.m_vWorldConnections.clear();
	m_vOldPrefabIDs.clear();
}

bool JsonSerializer::SetInstanceArrayProperty(rttr::instance& instance, const rttr::property& property, rttr::variant& variant, const rttr::type& variantType, const int& index)
{
	// If we are dealing with a array/vector/list with components
	if (!property.get_type().is_sequential_container())
		return false;

	// Create an array view
	rttr::variant var = instance.get_derived_type().get_property_value(property.get_name(), instance);
	rttr::variant_sequential_view arrView = var.create_sequential_view();

	// it is already the right size
	const size_t ContainerSize = arrView.get_size();

	if (ContainerSize > 0 && index < static_cast<const int>(ContainerSize))
	{
		// const bool isContainer = (arrView.get_value(0).extract_wrapped_value().is_associative_container() ||
			// arrView.get_value(0).extract_wrapped_value().is_sequential_container());

		if (variant.get_type().is_wrapper())
		{
			m_Logger.Log(LOGLEVEL_ERROR, "JsonSerializer", "Wrapped container items aren't supported, make sure policy::ctor::as_object is set on container items. See type: " + variant.get_type().get_name().to_string());
		}
		// if we are dealing with a member inside a component that is an entity
		else if (variantType == rttr::type::get<Entity*>() || Utils::CheckDerivedType(variantType, rttr::type::get<Entity*>()))
		{
			// convert it to the right entity type.
			variant.convert(variantType);
			if (!arrView.set_value(index, variant))
			{
				m_Logger.Log(LOGLEVEL_ERROR, "JsonSerializer", "Failed to set sequential container item. See type: " + variantType.get_name().to_string());
			}
		}
		// if we are dealing with a normal member inside a component that is a component
		else if (variantType == rttr::type::get<Component*>() || Utils::CheckDerivedType(variantType, rttr::type::get<Component*>()))
		{
			// if we are dealing with a member inside a component that is a entity
			Component* pTypeComponent = variant.get_value<Entity*>()->GetComponent(variantType.get_raw_type());
			rttr::variant varComponent = pTypeComponent;
			// convert it to the right component type.
			varComponent.convert(variantType);
			if (!arrView.set_value(index, varComponent))
			{
				m_Logger.Log(LOGLEVEL_ERROR, "JsonSerializer", "Failed to set sequential container item. See type: " + variantType.get_name().to_string());
			}
		}

		return SetInstanceProperty(instance, property, var, variantType, true);
	}

	return false;
}

bool JsonSerializer::SetInstanceProperty(rttr::instance& instance, const rttr::property& property, rttr::variant& variant, const rttr::type& variantType, bool bIsSequentialContainer)
{
	// if we are dealing with a member inside a component that is a entity
	if ( ( property.get_type() == rttr::type::get<Entity*>() || Utils::CheckDerivedType(property.get_type(), rttr::type::get<Entity*>()) ) || (Utils::CheckDerivedType(variantType, rttr::type::get<Entity*>()) && bIsSequentialContainer) )
	{
		// if we are dealing with a member inside a component that is a entity
		variant.convert(variantType);
		return instance.get_derived_type().set_property_value(property.get_name(), instance, variant);
	}
	 
	if (Utils::CheckDerivedType(property.get_type(), rttr::type::get<Component*>()) || (Utils::CheckDerivedType(variantType, rttr::type::get<Component*>()) && bIsSequentialContainer))
	{
		// if we are dealing with a member inside a component that is a entity
		rttr::variant newValueVariant;
		if (bIsSequentialContainer)
		{
			newValueVariant = variant;
			newValueVariant.convert(variantType);
		} 
		else
		{
			// We need to get the entity that has that component
			Entity* pEntityValue = variant.get_value<Entity*>();
			Component* pTypeComponent = (pEntityValue) ? pEntityValue->GetComponent(property.get_type().get_raw_type()) : nullptr;
			newValueVariant = pTypeComponent;
			newValueVariant.convert(property.get_type());
		}


		return instance.get_derived_type().set_property_value(property.get_name(), instance, newValueVariant);
	}
	
	// // if we are dealing with a member inside a component that is a entity
	// if (connection.rProperty.get_type() == rttr::type::get<Entity*>() || CheckDerivedType(connection.rProperty.get_type(), rttr::type::get<Entity*>()))
	// {
	// 	// if we are dealing with a member inside a component that is a entity
	// 	varEntity.convert(connection.rType);
	// 	if (!pCurrentComponent->get_type().set_property_value(connection.rProperty.get_name(), pCurrentComponent, varEntity))
	// 	{
	// 		m_Logger.Log(LOGLEVEL_ERROR, "JsonSerializer", "Could not connect entity or component");
	// 	}
	// }
	// else if (connection.rProperty.get_type() == rttr::type::get<Component*>() || CheckDerivedType(connection.rProperty.get_type(), rttr::type::get<Component*>()))
	// {
	// 	// if we are dealing with a member inside a component that is a entity
	// 	Component* pTypeComponent = varEntity.get_value<Entity*>()->GetComponent(connection.rProperty.get_type().get_raw_type());
	// 	rttr::variant varComponent = pTypeComponent;
	// 	varComponent.convert(connection.rProperty.get_type());
	// 	if (!pCurrentComponent->get_type().set_property_value(connection.rProperty.get_name(), pCurrentComponent, varComponent))
	// 	{
	// 		m_Logger.Log(LOGLEVEL_ERROR, "JsonSerializer", "Could not connect entity or component");
	// 	}
	// }

	return false;
}