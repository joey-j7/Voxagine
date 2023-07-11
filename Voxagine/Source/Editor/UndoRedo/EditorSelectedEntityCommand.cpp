#include "pch.h"

#include "Editor/UndoRedo/EditorSelectedEntityCommand.h"
#include "Editor/Editor.h"

#include "Editor/UndoRedo/EditorFunctionCommand.h"
#include "Editor/UndoRedo/CommandFunction.h"

EditorFunctionCommand * CreateEditorSelectedEntityCommand(Editor* pEditor, Entity * pNewSelectedEntity, Entity * pLastSelectedEntity)
{
	CommandFunction<Entity*>* RedoFunction = new CommandFunction<Entity*>(std::bind(&Editor::SetSelectedEntity, pEditor, std::placeholders::_1), pNewSelectedEntity);
	CommandFunction<Entity*>* UndoFunction = new CommandFunction<Entity*>(std::bind(&Editor::SetSelectedEntity, pEditor, std::placeholders::_1), pLastSelectedEntity);

	EditorFunctionCommand* NewEditorFunctionCommand = new EditorFunctionCommand(true, RedoFunction, UndoFunction);

	return NewEditorFunctionCommand;
}
