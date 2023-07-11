#pragma once

#include "Core/ECS/World.h"
#include "Editor/UndoRedo/CommandManager.h"

class Editor;
class Camera;
class EditorCamera;
class EditorEntityCommand;

class EditorWorld : public World
{
public:
	EditorWorld(Application* pApplication, Editor* pEditor, UVector2 chunkWorldSize = UVector2(1, 1));
	virtual ~EditorWorld();

	/* Setup functions */
	virtual void Initialize() override;
	virtual void Unload() override;

	/* Processes the add and remove queues for entities and components */
	virtual void PreTick() override;
	/* Processes Start and Tick functions on entities, components and systems */
	virtual void Tick(float fDeltaTime) override;
	/* Processes PostTick functions on entities and systems */
	virtual void PostTick(float fDeltaTime) override;

	/* Processes the FixedTick function for all entities and systems */
	virtual void FixedTick(const GameTimer& fixedTimer);

	/* Processes the PostFixedTick function for all entities and systems */
	virtual void PostFixedTick(const GameTimer& fixedTimer) override;

	void PrepareSerialization();
	void UnPrepareSerialization();

	Camera* SpawnDefaultPlayerCamera();
	Camera* GetPlayerCamera();

	bool HasEditor() const;
	Editor* GetEditor();

	bool IsMainCameraEditorCamera() const;
	bool HasEditorCamera() const;
	EditorCamera* GetEditorCamera() const;

	CommandManager& GetCommandManager();

	void AttachEntityToWorld(Entity* pEntity);
	Entity* DetachEntityFromWorld(Entity* pEntity);

	void AttachComponentToEntity(Entity* pEntity, Component* pComponent);
	Component* DetachComponentFromEntity(Entity* pEntity, Component* pComponent);

protected:
	void EmptyWorldInitialize();
private:
	std::vector<Entity*> m_pDetachedEntities;
	std::vector<Component*> m_pDetachedComponentsFromEntities;
	CommandManager m_CommandManager;

	Editor* m_pEditor = nullptr;
	EditorCamera* m_pEditorCamera = nullptr;

	bool m_bSpawnDefaultCamera = true;
	UVector2 m_ChunkWorldSize = UVector2(1, 1);
};