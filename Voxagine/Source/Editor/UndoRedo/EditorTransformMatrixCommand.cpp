#include "pch.h"

#include "Editor/UndoRedo/EditorTransformMatrixCommand.h"

#include "Editor/UndoRedo/EditorFunctionCommand.h"
#include "Editor/UndoRedo/CommandFunction.h"

EditorFunctionCommand * CreateEditorTransformMatrixCommand(Transform * pTransform, Matrix4& rNewMatrix, Matrix4& rLastMatrix)
{
	CommandFunction<Matrix4, bool>* RedoFunction = new CommandFunction<Matrix4, bool>(std::bind(&Transform::SetFromMatrix, pTransform, std::placeholders::_1, std::placeholders::_2), rNewMatrix, false);
	CommandFunction<Matrix4, bool>* UndoFunction = new CommandFunction<Matrix4, bool>(std::bind(&Transform::SetFromMatrix, pTransform, std::placeholders::_1, std::placeholders::_2), rLastMatrix, false);

	EditorFunctionCommand* NewEditorFunctionCommand = new EditorFunctionCommand(true, RedoFunction, UndoFunction);

	return NewEditorFunctionCommand;
}
