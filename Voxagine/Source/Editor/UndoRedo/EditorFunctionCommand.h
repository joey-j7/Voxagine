#pragma once

#include "Editor/UndoRedo/EditorStateCommand.h"

class CommandFunctionWrapper;

class EditorFunctionCommand : public EditorStateCommand
{
public:
	EditorFunctionCommand(bool bIsRedoExecuteCommand, CommandFunctionWrapper* pRedoFunction, CommandFunctionWrapper* pUndoFunction);
	virtual ~EditorFunctionCommand();

	virtual void Redo();
	virtual void Undo();

private:
	CommandFunctionWrapper* m_pRedoFunction;
	CommandFunctionWrapper* m_pUndoFunction;
};