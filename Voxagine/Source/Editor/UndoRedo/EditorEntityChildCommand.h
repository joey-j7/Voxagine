#pragma once

#include "Editor/UndoRedo/EditorFunctionCommand.h"

class Entity;
class World;

EditorFunctionCommand* CreateEntityAttachDetachParentCommand(bool bIsRedoExecuteCommand, Entity* pEntity, Entity* pParentEntity, Entity* pLastParentEntity = nullptr);
EditorFunctionCommand* CreateEntityAttachToParentCommand(Entity* pEntity, Entity* pParentEntity);
EditorFunctionCommand* CreateEntityDetachFromParentCommand(Entity* pEntity);

void AttachEntityToParent(World* pWorld, uint64_t EntityID, uint64_t ParentEntityID);
void DetachEntityFromParent(World* pWorld, uint64_t EntityID, uint64_t LastParentEntityID);