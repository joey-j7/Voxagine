#pragma once

#include "Editor/UndoRedo/EditorStateCommand.h"
#include "Core/JsonSerializer.h"
#include <External/rapidjson/document.h>
#include <External/rttr/type.h>

class Component;
class World;

class EditorComponentCommand : public EditorStateCommand
{
public:
	EditorComponentCommand(bool bIsbIsRedoExecuteCommand, bool bCreateWithEntityType, Component * pComponent, rttr::type ComponentType, Entity* pOwnerEntity);
	virtual ~EditorComponentCommand();

	virtual void Redo();
	virtual void Undo();

private:
	void AddComponentToEntity();
	void RemoveComponentFromEntity();

private:
	JsonSerializer* m_JsonSerializer = nullptr;
	rttr::type m_ComponentType;
	bool m_bCreateWithEntityType = false;
	
	uint64_t m_OwnerComponentID = -1u;
	World* m_pOwnerWorld = nullptr;

	Component* m_pNewCreatedComponent = nullptr;
	Value m_ComponentValue;
	Document m_ComponentDocument;
};

EditorComponentCommand* CreateComponentCreationCommand(rttr::type ComponentType, Entity* pOwnerEntity);
EditorComponentCommand* CreateComponentDestroyCommand(Component* pComponent);