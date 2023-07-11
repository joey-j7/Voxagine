#include "pch.h"
#include "EditorFunctionCommand.h"

#include "Editor/UndoRedo/CommandFunction.h"

EditorFunctionCommand::EditorFunctionCommand(bool bIsRedoExecuteCommand, CommandFunctionWrapper * pRedoFunction, CommandFunctionWrapper * pUndoFunction)
	: EditorStateCommand(bIsRedoExecuteCommand)
	, m_pRedoFunction(pRedoFunction)
	, m_pUndoFunction(pUndoFunction)
{

}

EditorFunctionCommand::~EditorFunctionCommand()
{
	if (m_pRedoFunction != nullptr)
		delete m_pRedoFunction;

	if (m_pUndoFunction != nullptr)
		delete m_pUndoFunction;
}

void EditorFunctionCommand::Redo()
{
	(IsRedoExecute()) ? m_pRedoFunction->ExecuteFunction() : m_pUndoFunction->ExecuteFunction();
}

void EditorFunctionCommand::Undo()
{
	(IsRedoExecute()) ? m_pUndoFunction->ExecuteFunction() : m_pRedoFunction->ExecuteFunction();
}
