#include "pch.h"
#include "Editor/UndoRedo/EditorComponentCommand.h"

#include "Core/Application.h"
#include "Core/ECS/Entity.h"
#include "Core/ECS/World.h"

EditorComponentCommand::EditorComponentCommand(bool bIsbIsRedoExecuteCommand, bool bCreateWithEntityType, Component * pComponent, rttr::type ComponentType, Entity* pOwnerEntity)
	: EditorStateCommand(bIsbIsRedoExecuteCommand)
	, m_ComponentType(ComponentType) // RTTR::Type doesn't contain a public constructor!
	, m_ComponentValue(kObjectType)
{
	m_JsonSerializer = (pComponent != nullptr) ? &pComponent->GetOwner()->GetWorld()->GetApplication()->GetSerializer() : &pOwnerEntity->GetWorld()->GetApplication()->GetSerializer();
	m_bCreateWithEntityType = bCreateWithEntityType;

	m_OwnerComponentID = (pComponent != nullptr) ? pComponent->GetOwner()->GetId() : pOwnerEntity->GetId();
	m_pOwnerWorld = (pComponent != nullptr) ? pComponent->GetOwner()->GetWorld() : pOwnerEntity->GetWorld();

	m_pNewCreatedComponent = pComponent;

	if (!bCreateWithEntityType)
	{
		m_JsonSerializer = &m_pOwnerWorld->GetApplication()->GetSerializer();
		m_JsonSerializer->ComponentToValue(pComponent, m_ComponentValue, m_ComponentDocument.GetAllocator());
	}
}

EditorComponentCommand::~EditorComponentCommand()
{
}

void EditorComponentCommand::Redo()
{
	(IsRedoExecute()) ? AddComponentToEntity() : RemoveComponentFromEntity();
}

void EditorComponentCommand::Undo()
{
	(IsRedoExecute()) ? RemoveComponentFromEntity() : AddComponentToEntity();
}

void EditorComponentCommand::AddComponentToEntity()
{
	Component* pNewComponent = nullptr;
	Entity* pOwnerEntity = m_pOwnerWorld->FindEntity(m_OwnerComponentID);

	if (!m_bCreateWithEntityType)
	{
		pNewComponent = m_JsonSerializer->ValueToComponent(*m_pOwnerWorld, m_ComponentValue, pOwnerEntity);
		//m_pOwnerWorld->GetApplication()->GetLoggingSystem().Log(LOGLEVEL_MESSAGE, "Undo/Redo", "Component re-created and added to entity ID: " + std::to_string(pOwnerEntity->GetId()) + " of the component (type): " + m_ComponentType.get_name().to_string());
	}
	else
	{
		rttr::variant ComponentVariant = m_ComponentType.create({ pOwnerEntity });
		pNewComponent = ComponentVariant.get_value<Component*>();

		//m_pOwnerWorld->GetApplication()->GetLoggingSystem().Log(LOGLEVEL_MESSAGE, "Undo/Redo", "Component created and added to entity ID: " + std::to_string(pOwnerEntity->GetId()) + " of the component (type): " + m_ComponentType.get_name().to_string());
	}

	m_pNewCreatedComponent = pNewComponent;
	pOwnerEntity->AddComponent(pNewComponent);

	if (m_bCreateWithEntityType)
	{
		m_ComponentValue = Value(kObjectType);
		m_ComponentDocument = Document();

		m_JsonSerializer->ComponentToValue(pNewComponent, m_ComponentValue, m_ComponentDocument.GetAllocator());
		m_bCreateWithEntityType = false;
	}
}

void EditorComponentCommand::RemoveComponentFromEntity()
{
	Entity* pOwnerEntity = m_pOwnerWorld->FindEntity(m_OwnerComponentID);
	pOwnerEntity->RemoveComponent(m_pNewCreatedComponent);

	//m_pOwnerWorld->GetApplication()->GetLoggingSystem().Log(LOGLEVEL_MESSAGE, "Undo/Redo", "Component destroyed from entity ID: " + std::to_string(pOwnerEntity->GetId()) + " of the component (type): " + m_ComponentType.get_name().to_string());
	m_pNewCreatedComponent = nullptr;
}

EditorComponentCommand * CreateComponentCreationCommand(rttr::type ComponentType, Entity* pOwnerEntity)
{
	return new EditorComponentCommand(true, true, nullptr, ComponentType, pOwnerEntity);
}

EditorComponentCommand * CreateComponentDestroyCommand(Component * pComponent)
{
	return new EditorComponentCommand(false, false, pComponent, rttr::instance(pComponent).get_derived_type(), pComponent->GetOwner());
}
