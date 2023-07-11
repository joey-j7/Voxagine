#pragma once

#include "Editor/UndoRedo/EditorFunctionCommand.h"

class Editor;
class Entity;

EditorFunctionCommand* CreateEditorSelectedEntityCommand(Editor* pEditor, Entity* pNewSelectedEntity, Entity* pLastSelectedEntity);