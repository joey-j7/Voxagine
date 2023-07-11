#include "pch.h"
#include "EditorRenderMapper.h"

#include "Editor/Editor.h"
#include "Core/Application.h"
#include "Core/ECS/Components/VoxRenderer.h"
#include "Core/ECS/WorldManager.h"

void EditorRenderMapper::Initialize(Editor* pEditor)
{
	m_pEditor = pEditor;

	// Register to the world loaded/created event
	pEditor->GetApplication()->GetWorldManager().WorldLoaded += Event<World*>::Subscriber(std::bind(&EditorRenderMapper::OnWorldLoaded, this, std::placeholders::_1), this);

	// Register to the world unloaded/popped event
	pEditor->GetApplication()->GetWorldManager().WorldPopped += Event<World*>::Subscriber(std::bind(&EditorRenderMapper::OnWorldPopped, this, std::placeholders::_1), this);
}

void EditorRenderMapper::Tick()
{
	if (m_pEditor->GetEditorModus() == EM_EDITOR)
	{
		if (m_bSmartMapping)
		{
			World* pWorld = m_pEditor->GetApplication()->GetWorldManager().GetTopWorld();
			if (pWorld)
			{
				UpdateRenderMap(pWorld);
			}
		}
	}
}

void EditorRenderMapper::UpdateRenderMap(World* pWorld)
{
	RenderSystem* pRenderSystem = pWorld->GetRenderSystem();

	for (auto& pair : m_bakedRenderers[pWorld])
	{
		if (pair.first->GetTransform()->IsUpdated())
		{
			if (VoxRenderer* pRenderComp = pair.first->GetComponent<VoxRenderer>())
			{
				pRenderSystem->m_VoxelBaker.Clear(pRenderComp, &pair.second);
				pRenderSystem->m_VoxelBaker.Occupy(pRenderComp, &pair.second);
			}
		}
	}
}

void EditorRenderMapper::EntityToRenderMap(Entity* pEntity)
{
	auto findIter = m_bakedRenderers.find(pEntity->GetWorld());
	if (findIter != m_bakedRenderers.end())
	{
		if (pEntity->IsStatic())
		{
			auto findEntityIter = findIter->second.find(pEntity);
			if (findEntityIter == findIter->second.end())
			{
				(*findIter).second[pEntity] = VoxRenderer::BakeData();
				
				if (VoxRenderer* pRenderComp = pEntity->GetComponent<VoxRenderer>())
				{
					RenderSystem* pRenderSystem = pEntity->GetWorld()->GetRenderSystem();
					pRenderSystem->m_VoxelBaker.Occupy(pRenderComp, &(*findIter).second[pEntity]);
				}
			}
		}
		else
		{
			OnEntityRemoved(pEntity);
		}
	}
}

void EditorRenderMapper::OnEntityAdded(Entity* pEntity)
{
	EntityToRenderMap(pEntity);
	pEntity->StaticPropertyChanged += Event<Entity*, bool>::Subscriber(
		std::bind(&EditorRenderMapper::OnEntityStaticChanged, this, std::placeholders::_1, std::placeholders::_2), this);
}

void EditorRenderMapper::OnEntityRemoved(Entity* pEntity)
{
	auto findIter = m_bakedRenderers.find(pEntity->GetWorld());
	if (findIter != m_bakedRenderers.end())
	{
		auto findEntityIter = findIter->second.find(pEntity);

		if (findEntityIter != findIter->second.end())
		{
			if (VoxRenderer* pRenderComp = pEntity->GetComponent<VoxRenderer>())
			{
				pEntity->GetWorld()->GetRenderSystem()->m_VoxelBaker.Clear(pRenderComp, &findEntityIter->second);
			}
			
			findIter->second.erase(findEntityIter);
		}
	}
}

void EditorRenderMapper::OnWorldLoaded(World* pWorld)
{
	if (m_pEditor->GetEditorModus() == EditorModus::EM_EDITOR)
	{
		m_bakedRenderers[pWorld] = std::unordered_map<Entity*, VoxRenderer::BakeData>();

		pWorld->EntityAdded += Event<Entity*>::Subscriber(std::bind(&EditorRenderMapper::OnEntityAdded, this, std::placeholders::_1), this);
		pWorld->EntityRemoved += Event<Entity*>::Subscriber(std::bind(&EditorRenderMapper::OnEntityRemoved, this, std::placeholders::_1), this);

		for (Entity* pEntity : pWorld->GetEntities())
		{
			EntityToRenderMap(pEntity);
			pEntity->StaticPropertyChanged += Event<Entity*, bool>::Subscriber(
				std::bind(&EditorRenderMapper::OnEntityStaticChanged, this, std::placeholders::_1, std::placeholders::_2), this);
		}
	}
}

void EditorRenderMapper::OnWorldPopped(World* pWorld)
{
	auto findIter = m_bakedRenderers.find(pWorld);
	if (findIter != m_bakedRenderers.end())
	{
		for (auto& pair : findIter->second)
		{
			delete[] pair.second.Positions;
			pair.second.Positions = nullptr;
		}

		m_bakedRenderers.erase(findIter);
	}
}

void EditorRenderMapper::OnEntityStaticChanged(Entity* pEntity, bool isStatic)
{
	EntityToRenderMap(pEntity);
}
