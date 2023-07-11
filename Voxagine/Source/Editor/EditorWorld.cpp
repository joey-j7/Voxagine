#include "pch.h"
#include "EditorWorld.h"

#include "Core/Application.h"
#include "Core/Platform/Platform.h"
#include "Core/Platform/Rendering/RenderContext.h"

#include <Core/ECS/Systems/ScriptSystem.h>
#include <Core/ECS/Systems/Physics/PhysicsSystem.h>
#include <Core/ECS/Systems/Rendering/RenderSystem.h>
#include "Core/ECS/Systems/AudioSystem.h"
#include "Core/ECS/Systems/Chunk/ChunkSystem.h"

#include "Editor/Editor.h"
#include "Editor/EditorCamera.h"

EditorWorld::EditorWorld(Application* pApplication, Editor* pEditor, UVector2 chunkWorldSize /*= UVector2(1, 1) */)
	: World(pApplication)
	, m_pEditor(pEditor)
	, m_CommandManager()
	, m_ChunkWorldSize(chunkWorldSize)
{
}

EditorWorld::~EditorWorld()
{
}

void EditorWorld::Initialize()
{
	m_JobQueueHandle = GetApplication()->GetJobManager().CreateJobQueue();

	if (!IsPreLoaded())
		EmptyWorldInitialize();

	m_pEditorCamera = new EditorCamera(this, GetEditor());
	EditorModus CurrentEditorModus = GetEditor()->GetEditorModus();

	if (CurrentEditorModus == EditorModus::EM_EDITOR)
	{
		Camera* PlayerCamera = GetPlayerCamera();

		if (PlayerCamera != nullptr)
		{
			PlayerCamera->Recalculate();
			m_pEditorCamera->GetTransform()->SetFromMatrix(PlayerCamera->GetTransform()->GetMatrix());
			m_bSpawnDefaultCamera = false;
		}
		else
		{
			UVector3 worldSize;
			GetApplication()->GetWorldManager().GetTopWorld()->GetVoxelGrid()->GetDimensions(worldSize.x, worldSize.y, worldSize.z);
			m_pEditorCamera->GetTransform()->SetPosition(Vector3(static_cast<float>(worldSize.x * 0.5f), static_cast<float>(worldSize.y * 2.0f), -static_cast<float>(worldSize.x * 0.75f)));
			m_pEditorCamera->GetTransform()->SetEulerAngles(Vector3(45, 0, 0));
			m_bSpawnDefaultCamera = true;
		}

		SetMainCamera(m_pEditorCamera);
	}
	else if (CurrentEditorModus == EditorModus::EM_PLAY)
	{
		Camera* PlayerCamera = GetPlayerCamera();

		if (PlayerCamera == nullptr)
		{
			PlayerCamera = SpawnDefaultPlayerCamera();
			m_bSpawnDefaultCamera = true;
		}

		SetMainCamera(PlayerCamera);
	}

	// Start all systems
	for (ComponentSystem* pSystem : m_Systems)
		pSystem->Start();

	GetRenderSystem()->Start();
}

void EditorWorld::Unload()
{
	World::Unload();

	if (HasEditorCamera())
		delete m_pEditorCamera;
}

void EditorWorld::PreTick()
{
	for (Component* pDetachedComponent : m_pDetachedComponentsFromEntities)
	{
		Entity* pDetachedComponentOwner = pDetachedComponent->GetOwner();
		std::vector<Component*>& EntityRemovedComponentList = GetRemovedComponentsFromEntity(pDetachedComponentOwner);

		DeleteComponentFromEntityLists(pDetachedComponent->GetOwner(), pDetachedComponent);

		const std::vector<Component*>::iterator iter = std::find(EntityRemovedComponentList.begin(), EntityRemovedComponentList.end(), pDetachedComponent);

		if (iter != EntityRemovedComponentList.end())
		{
			for (Component* pAliveComponent : pDetachedComponent->GetOwner()->GetComponents())
			{
				if (pAliveComponent == pDetachedComponent)
				{
					continue;
				}
				else
				{
					pAliveComponent->DependencyCheck(pDetachedComponent, false);
				}
			}

			EntityRemovedComponentList.erase(iter);
		}
	}

	m_pDetachedComponentsFromEntities.clear();
	for (Entity* pDetachedEntity : m_pDetachedEntities)
	{
		DeleteEntityFromLists(pDetachedEntity);

		const std::vector<Entity*>::iterator iter = std::find(GetRemovedEntities().begin(), GetRemovedEntities().end(), pDetachedEntity);

		if (iter != GetRemovedEntities().end())
			GetRemovedEntities().erase(iter);
	}

	m_pDetachedEntities.clear();

	World::PreTick();

	if (IsMainCameraEditorCamera())
		GetEditorCamera()->PreTick();
}

void EditorWorld::Tick(float fDeltaTime)
{
	if (GetEditor()->GetEditorModus() == EditorModus::EM_PLAY)
		World::Tick(fDeltaTime);

	if (IsMainCameraEditorCamera())
		GetEditorCamera()->Tick(fDeltaTime);
}

void EditorWorld::PostTick(float fDeltaTime)
{
	if (GetEditor()->GetEditorModus() == EditorModus::EM_PLAY)
	{
		World::PostTick(fDeltaTime);
	}
	else
	{
		m_pPhysicsSystem->PostTick(fDeltaTime);
		m_pRenderSystem->PostTick(fDeltaTime);
		m_pAudioSystem->PostTick(fDeltaTime);
		m_pChunkSystem->PostTick(fDeltaTime);
	}

	if (IsMainCameraEditorCamera())
		GetEditorCamera()->PostTick(fDeltaTime);
}

void EditorWorld::FixedTick(const GameTimer& fixedTimer)
{
	if (GetEditor()->GetEditorModus() == EditorModus::EM_PLAY)
		World::FixedTick(fixedTimer);
	else
	{
		if (m_pChunkSystem)
			m_pChunkSystem->FixedTick(fixedTimer);
	}

	if (IsMainCameraEditorCamera())
		GetEditorCamera()->FixedTick(fixedTimer);
}

void EditorWorld::PostFixedTick(const GameTimer& fixedTimer)
{
	if (GetEditor()->GetEditorModus() == EditorModus::EM_PLAY)
	{
		World::PostFixedTick(fixedTimer);
	}
	else
	{
		m_pPhysicsSystem->PostFixedTick(fixedTimer);
		m_pRenderSystem->PostFixedTick(fixedTimer);
	}

	if (IsMainCameraEditorCamera())
		GetEditorCamera()->PostFixedTick(fixedTimer);
}

void EditorWorld::PrepareSerialization()
{
	Camera* PlayerCamera = GetPlayerCamera();

	if (PlayerCamera == nullptr)
	{
		PlayerCamera = SpawnDefaultPlayerCamera();
		m_bSpawnDefaultCamera = true;
	}

	SetMainCamera(PlayerCamera);
}

void EditorWorld::UnPrepareSerialization()
{
	if (m_bSpawnDefaultCamera)
	{
		Camera* PlayerCamera = GetPlayerCamera();

		if (PlayerCamera != nullptr)
			RemoveEntity(PlayerCamera);

		m_bSpawnDefaultCamera = false;
	}

	if (HasEditorCamera())
	{
		SetMainCamera(GetEditorCamera());
	}
}

Camera* EditorWorld::SpawnDefaultPlayerCamera()
{
	Camera* PlayerCamera = GetPlayerCamera();

	if (PlayerCamera == nullptr)
	{
		PlayerCamera = new Camera(this);
		PlayerCamera->SetName("Player Camera");

		UVector3 worldSize;
		GetApplication()->GetWorldManager().GetTopWorld()->GetVoxelGrid()->GetDimensions(worldSize.x, worldSize.y, worldSize.z);

		PlayerCamera->GetTransform()->SetPosition(Vector3(static_cast<float>(worldSize.x * 0.5f), static_cast<float>(worldSize.y * 2.0f), -static_cast<float>(worldSize.x * 0.75f)));
		PlayerCamera->GetTransform()->SetEulerAngles(Vector3(45, 0, 0));

		AddEntity(PlayerCamera);
	}

	return PlayerCamera;
}

Camera* EditorWorld::GetPlayerCamera()
{
	if (GetMainCamera() != nullptr)
	{
		if (dynamic_cast<Camera*>(GetMainCamera()) && !dynamic_cast<EditorCamera*>(GetMainCamera()))
			return GetMainCamera();
	}

	const std::vector<Entity*>& WorldEntities = GetEntities();

	for (Entity* it : WorldEntities)
	{
		if (dynamic_cast<Camera*>(it) && !dynamic_cast<EditorCamera*>(it))
		{
			return static_cast<Camera*>(it);
		}
	}

	return nullptr;
}

bool EditorWorld::HasEditor() const
{
	return (m_pEditor != nullptr);
}

Editor * EditorWorld::GetEditor()
{
	return m_pEditor;
}

bool EditorWorld::IsMainCameraEditorCamera() const
{
	return (HasEditorCamera()) ? GetMainCamera() == GetEditorCamera() : false;
}

bool EditorWorld::HasEditorCamera() const
{
	return (GetEditorCamera() != nullptr);
}

EditorCamera * EditorWorld::GetEditorCamera() const
{
	return m_pEditorCamera;
}

CommandManager & EditorWorld::GetCommandManager()
{
	return m_CommandManager;
}

void EditorWorld::AttachEntityToWorld(Entity * pEntity)
{
	AddRemovedEntityToWorld(pEntity);
}

Entity * EditorWorld::DetachEntityFromWorld(Entity * pEntity)
{
	Entity* pDetachedEntity = RemoveEntityFromWorld(pEntity);

	if (pDetachedEntity != nullptr)
		m_pDetachedEntities.push_back(pEntity);

	return pDetachedEntity;
}

void EditorWorld::AttachComponentToEntity(Entity * pEntity, Component * pComponent)
{
	if (pEntity->GetWorld() == this)
		pEntity->AddComponent(pComponent);
}

Component * EditorWorld::DetachComponentFromEntity(Entity * pEntity, Component * pComponent)
{
	const std::vector<Component*>::const_iterator iter = std::find(pComponent->GetOwner()->GetComponents().begin(), pComponent->GetOwner()->GetComponents().end(), pComponent);

	if (iter != pComponent->GetOwner()->GetComponents().end())
	{
		pComponent->GetOwner()->RemoveComponent(pComponent);
		pComponent->Destroyed(pComponent);
		m_pDetachedComponentsFromEntities.push_back(pComponent);
		return pComponent;
	}

	return nullptr;
}

void EditorWorld::EmptyWorldInitialize()
{
	UVector2 chunkSize = UVector2(256, 256);
	UVector3 voxelGridSize(chunkSize.x, 128, chunkSize.y);
	if (m_ChunkWorldSize.x > 1)
		voxelGridSize.x = 768;
	if (m_ChunkWorldSize.y > 1)
		voxelGridSize.z = 768;

	std::vector<ComponentSystem*> Systems;

	ScriptSystem* pScriptSystem = new ScriptSystem(this);
	PhysicsSystem* pPhysicsSystem = new PhysicsSystem(this, voxelGridSize, 1, 150000, Vector3(chunkSize.x, 128, chunkSize.y));
	SetPhysicsSystem(pPhysicsSystem);

	AudioSystem* pAudioSystem = new AudioSystem(this);
	SetAudioSystem(pAudioSystem);

	std::unordered_map<uint32_t, Chunk*> chunks;
	chunks.reserve(m_ChunkWorldSize.x * m_ChunkWorldSize.y);
	for (uint32_t i = 0; i < m_ChunkWorldSize.x * m_ChunkWorldSize.y; ++i)
	{
		UVector2 chunkId(floor(i / m_ChunkWorldSize.y), i % m_ChunkWorldSize.y);
		chunks[i] = new Chunk(GetApplication(), this, chunkId);
	}

	ChunkSystem* pChunkSystem = new ChunkSystem(this, chunks, chunkSize, m_ChunkWorldSize * chunkSize);
	SetChunkSystem(pChunkSystem);

	Systems.push_back(pScriptSystem);
	Systems.push_back(pPhysicsSystem);
	Systems.push_back(pAudioSystem);
	Systems.push_back(pChunkSystem);

	SetSystems(Systems);
	SetRenderSystem(new RenderSystem(this));
}
