#pragma once

#include "Editor/UndoRedo/EditorStateCommand.h"
#include "Core/JsonSerializer.h"
#include <External/rapidjson/document.h>
#include <External/rttr/type.h>

class JsonSerializer;
class Entity;
class World;
class Editor;

class EditorEntityCommand : public EditorStateCommand
{
public:
	virtual ~EditorEntityCommand();

	friend EditorEntityCommand* CreateDuplicateEntityCreationCommand(Editor* pEditor, Entity* pEntity);
	friend EditorEntityCommand* CreateEntityCreationCommand(Editor* pEditor, rttr::type pEntityType, World* pWorld, Entity* pParentEntity);
	friend EditorEntityCommand* CreateEntityDestroyCommand(Editor* pEditor, Entity* pEntity);

	virtual void Redo();
	virtual void Undo();

	Entity* GetCreatedEntity();
private:
	EditorEntityCommand(Editor* pEditor, bool bIsbIsRedoExecuteCommand, bool bCreateWithEntityType, bool bDuplicateEntity, Entity* pTargetEntity, rttr::type EntityType, World* pEntityWorld, Entity* pParentEntity);

	void AddEntityToWorld();
	void RemoveEntityFromWorld();

	void UpdateComponentsRecursive(Entity* pTargetEntity);
	void RegisterComponentsRecursive(Entity* pTargetEntity);

	void IncrementEntityName() const;
private:
	Editor* m_pEditor = nullptr;
	JsonSerializer* m_JsonSerializer = nullptr;
	rttr::type m_EntityType;
	bool m_bCreateWithEntityType = false;
	bool m_bDuplicateEntity = false;

	uint64_t m_TargetEntityID = static_cast<uint64_t>(-1);
	World* m_pEntityWorld = nullptr;
	uint64_t m_ParentEntityID = static_cast<uint64_t>(-1);

	Entity* m_pNewCreatedEntity;
	mutable std::string m_EntityNameCustom;

	bool m_bHasEntityCopy = false;
	Value m_EntityValue;
	Document m_EntityDocument;
};

EditorEntityCommand* CreateDuplicateEntityCreationCommand(Editor* pEditor, Entity* pEntity);
EditorEntityCommand* CreateEntityCreationCommand(Editor* pEditor, rttr::type pEntityType, World* pWorld, Entity* pParentEntity);
EditorEntityCommand* CreateEntityDestroyCommand(Editor* pEditor, Entity* pEntity);