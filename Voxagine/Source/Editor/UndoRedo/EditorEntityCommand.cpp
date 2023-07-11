#include "pch.h"
#include "EditorEntityCommand.h"


#include "Editor/Editor.h"

#include "Core/Application.h"
#include "Core/ECS/Entity.h"
#include "Core/ECS/World.h"

EditorEntityCommand::~EditorEntityCommand()
{
}

void EditorEntityCommand::Redo()
{
	(IsRedoExecute()) ? AddEntityToWorld() : RemoveEntityFromWorld();
}

void EditorEntityCommand::Undo()
{
	(IsRedoExecute()) ? RemoveEntityFromWorld() : AddEntityToWorld();
}

Entity * EditorEntityCommand::GetCreatedEntity()
{
	return m_pNewCreatedEntity;
}

EditorEntityCommand::EditorEntityCommand(Editor* pEditor, bool bIsbIsRedoExecuteCommand, bool bCreateWithEntityType, bool bDuplicateEntity, Entity* pTargetEntity, rttr::type EntityType, World* pEntityWorld, Entity* pParentEntity)
	: EditorStateCommand(bIsbIsRedoExecuteCommand)
	, m_pEditor(pEditor)
	, m_EntityType(EntityType) // RTTR::Type doesn't contain a public constructor!
	, m_EntityValue(kObjectType)
{
	m_JsonSerializer = &pEntityWorld->GetApplication()->GetSerializer();
	m_bCreateWithEntityType = bCreateWithEntityType;
	m_bDuplicateEntity = bDuplicateEntity;

	m_TargetEntityID = (pTargetEntity != nullptr) ? pTargetEntity->GetId() : -1;
	m_pEntityWorld = pEntityWorld;
	m_ParentEntityID = (pParentEntity != nullptr) ? pParentEntity->GetId() : -1;

	if (m_bDuplicateEntity && pTargetEntity != nullptr)
		m_EntityNameCustom = pTargetEntity->GetName();
}

void EditorEntityCommand::AddEntityToWorld()
{
	Entity* pNewEntity = nullptr;
	Entity* pNewEntityParent = (m_ParentEntityID != -1) ? m_pEntityWorld->FindEntity(m_ParentEntityID) : nullptr;
	bool bTempDuplicateEntity = m_bDuplicateEntity;

	if (!m_bCreateWithEntityType)
	{
		if (m_bDuplicateEntity)
		{
			Document* entityCopyDocument;
			Value* entityValueDocument;
			m_pEditor->GetCopyEntityData(entityCopyDocument, entityValueDocument);

			pNewEntity = m_JsonSerializer->ValueToEntity(*entityValueDocument, *m_pEntityWorld, true, pNewEntityParent);
			m_bDuplicateEntity = false;
			IncrementEntityName();

			//m_pEntityWorld->GetApplication()->GetLoggingSystem().Log(LOGLEVEL_MESSAGE, "Undo/Redo", "Entity duplicated with ID: " + std::to_string(pNewEntity->GetId()));
		}
		else
		{
			pNewEntity = m_JsonSerializer->ValueToEntity(m_EntityValue, *m_pEntityWorld, false, pNewEntityParent);
			//m_pEntityWorld->GetApplication()->GetLoggingSystem().Log(LOGLEVEL_MESSAGE, "Undo/Redo", "Entity re-created with ID: " + std::to_string(pNewEntity->GetId()));
		}

		pNewEntity->SetName(m_EntityNameCustom);
	}
	else
	{
		rttr::variant EntityVariant = m_EntityType.create({ m_pEntityWorld });
		pNewEntity = EntityVariant.get_value<Entity*>();

		//m_pEntityWorld->GetApplication()->GetLoggingSystem().Log(LOGLEVEL_MESSAGE, "Undo/Redo", "Entity created with ID: " + std::to_string(pNewEntity->GetId()));
	}

	m_TargetEntityID = pNewEntity->GetId();
	m_pNewCreatedEntity = pNewEntity;

	m_pEntityWorld->AddEntity(pNewEntity);
// 	if (pNewEntityParent != nullptr)
// 		pNewEntity->SetParent(pNewEntityParent);

	for (Entity* pChild : pNewEntity->GetChildren())
		m_pEntityWorld->AddEntity(pChild);

	if (m_bCreateWithEntityType || bTempDuplicateEntity)
	{
		m_EntityValue = Value(kObjectType);
		m_EntityDocument = Document();

		m_JsonSerializer->EntityToValue(pNewEntity, m_EntityValue, m_EntityDocument.GetAllocator());
		m_bCreateWithEntityType = false;
		m_bHasEntityCopy = true;
	}

	m_pEditor->SetSelectedEntity(m_pNewCreatedEntity);
}

void EditorEntityCommand::RemoveEntityFromWorld()
{
	Entity* pFoundTargetEntity = m_pEntityWorld->FindEntity(m_TargetEntityID);
	Entity* pTargetEntity = (pFoundTargetEntity != nullptr) ? pFoundTargetEntity : (m_pNewCreatedEntity->GetId() == m_TargetEntityID) ? m_pNewCreatedEntity : nullptr;

	if (pTargetEntity == nullptr)
	{
		//m_pEntityWorld->GetApplication()->GetLoggingSystem().Log(LOGLEVEL_ERROR, "Undo/Redo", "Failed to find entity to remove!" + std::to_string(m_TargetEntityID));
		return;
	}

	m_EntityNameCustom = pTargetEntity->GetName();

	if (!m_bHasEntityCopy)
	{
		UpdateComponentsRecursive(pTargetEntity);
		m_JsonSerializer->EntityToValue(pTargetEntity, m_EntityValue, m_EntityDocument.GetAllocator());
		m_bHasEntityCopy = true;
	}

	m_pEntityWorld->RemoveEntity(pTargetEntity);
	m_pNewCreatedEntity = nullptr;

	//m_pEntityWorld->GetApplication()->GetLoggingSystem().Log(LOGLEVEL_MESSAGE, "Undo/Redo", "Entity destroyed with ID: " + std::to_string(m_TargetEntityID));
}

void EditorEntityCommand::UpdateComponentsRecursive(Entity * pTargetEntity)
{
	pTargetEntity->UpdateComponents();
	for (Entity* pChild : pTargetEntity->GetChildren())
	{
		UpdateComponentsRecursive(pChild);
	}
}

void EditorEntityCommand::RegisterComponentsRecursive(Entity * pTargetEntity)
{
	pTargetEntity->GetWorld()->RegisterComponents(pTargetEntity->GetPreTickAddedComponents());
	for (Entity* pChild : pTargetEntity->GetChildren())
	{
		RegisterComponentsRecursive(pChild);
	}
}

void EditorEntityCommand::IncrementEntityName() const
{
	if (m_EntityNameCustom.empty())
	{
		m_EntityNameCustom = m_EntityNameCustom + "_1";
		return;
	}

	size_t stringindex = m_EntityNameCustom.size() - 1;
	bool ExitLoop = false;

	while (!ExitLoop)
	{
		if (std::atoll(&m_EntityNameCustom[stringindex]) == 0 && m_EntityNameCustom[stringindex] != char('0'))
		{
			ExitLoop = true;
		}
		else
		{
			--stringindex;
		}
	}

	if (stringindex == 0)
	{
		m_EntityNameCustom = m_EntityNameCustom + "_1";
		return;
	}

	long long currentvalue = std::atoll(m_EntityNameCustom.substr(stringindex + 1, m_EntityNameCustom.size()).c_str());
	m_EntityNameCustom.replace(stringindex + 1, m_EntityNameCustom.size(), std::to_string(++currentvalue));
}

EditorEntityCommand* CreateDuplicateEntityCreationCommand(Editor* pEditor, Entity* pEntity)
{
	return new EditorEntityCommand(pEditor, true, false, true, pEntity, rttr::instance(pEntity).get_derived_type(), pEntity->GetWorld(), pEntity->GetParent());
};

EditorEntityCommand* CreateEntityCreationCommand(Editor* pEditor, rttr::type EntityType, World* pWorld, Entity* pParentEntity)
{
	return new EditorEntityCommand(pEditor, true, true, false, nullptr, EntityType, pWorld, pParentEntity);
};

EditorEntityCommand* CreateEntityDestroyCommand(Editor* pEditor, Entity* pEntity)
{
	return new EditorEntityCommand(pEditor, false, false, false, pEntity, rttr::instance(pEntity).get_derived_type(), pEntity->GetWorld(), pEntity->GetParent());
};