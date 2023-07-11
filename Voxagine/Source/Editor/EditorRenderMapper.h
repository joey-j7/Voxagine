#pragma once
#include <unordered_map>
#include "Core/ECS/Systems/Rendering/RenderSystem.h"

#include "Core/ECS/Components/VoxRenderer.h"

class Editor;
class EditorRenderMapper
{
public:
	EditorRenderMapper() {}

	void Initialize(Editor* pEditor);
	void Tick();

	void UpdateRenderMap(World* pWorld);
	void SetSmartMapping(bool bSmartMapping) { m_bSmartMapping = bSmartMapping; }

private:
	Editor* m_pEditor = nullptr;
	bool m_bSmartMapping = true;

	std::unordered_map<World*, std::unordered_map<Entity*, VoxRenderer::BakeData>> m_bakedRenderers;

	void EntityToRenderMap(Entity* pEntity);

	void OnEntityAdded(Entity* pEntity);
	void OnEntityRemoved(Entity* pEntity);

	void OnWorldLoaded(World* pWorld);
	void OnWorldPopped(World* pWorld);

	void OnEntityStaticChanged(Entity* pEntity, bool isStatic);
};