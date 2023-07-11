#include "pch.h"
#include "Editor/UndoRedo/EditorEntityChildCommand.h"

#include "Editor/UndoRedo/EditorFunctionCommand.h"
#include "Editor/UndoRedo/CommandFunction.h"

#include "Core/ECS/Entity.h"
#include "Core/ECS/World.h"

EditorFunctionCommand * CreateEntityAttachDetachParentCommand(bool bIsRedoExecuteCommand, Entity* pEntity, Entity* pParentEntity, Entity* pLastParentEntity)
{
	CommandFunction<World*, uint64_t, uint64_t>* RedoFunction = new CommandFunction<World*, uint64_t, uint64_t>(AttachEntityToParent, pEntity->GetWorld(), pEntity->GetId(), (pParentEntity != nullptr) ? pParentEntity->GetId() : -1);
	CommandFunction<World*, uint64_t, uint64_t>* UndoFunction = new CommandFunction<World*, uint64_t, uint64_t>(DetachEntityFromParent, pEntity->GetWorld(), pEntity->GetId(), (pEntity->GetParent() != nullptr) ? pEntity->GetParent()->GetId() : -1 );

	EditorFunctionCommand* NewEditorFunctionCommand = new EditorFunctionCommand(bIsRedoExecuteCommand, RedoFunction, UndoFunction);

	return NewEditorFunctionCommand;
}

EditorFunctionCommand * CreateEntityAttachToParentCommand(Entity* pEntity, Entity* pParentEntity)
{
	return CreateEntityAttachDetachParentCommand(true, pEntity, pParentEntity, pEntity->GetParent());
}

EditorFunctionCommand * CreateEntityDetachFromParentCommand(Entity* pEntity)
{
	return CreateEntityAttachDetachParentCommand(true, pEntity, nullptr, pEntity->GetParent());
}

void AttachEntityToParent(World * pWorld, uint64_t EntityID, uint64_t ParentEntityID)
{
	Entity* pTargetEntity = pWorld->FindEntity(EntityID);
	Entity* pParentEntity = pWorld->FindEntity(ParentEntityID);

	pTargetEntity->SetParent(pParentEntity);
}

void DetachEntityFromParent(World * pWorld, uint64_t EntityID, uint64_t LastParentEntityID)
{
	Entity* pTargetEntity = pWorld->FindEntity(EntityID);
	Entity* pParentEntity = (LastParentEntityID != -1) ? pWorld->FindEntity(LastParentEntityID) : nullptr;

	pTargetEntity->SetParent(pParentEntity);
}
