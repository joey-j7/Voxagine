#pragma once

#include "Editor/UndoRedo/BaseCommand.h"

class EditorStateCommand : public BaseCommand
{
public:
	EditorStateCommand(bool bIsRedoExecuteCommand) : m_bIsRedoExecuteCommand(bIsRedoExecuteCommand) {};
	virtual ~EditorStateCommand() {};

	bool IsRedoExecute() const { return m_bIsRedoExecuteCommand; };

private:
	bool m_bIsRedoExecuteCommand;
};