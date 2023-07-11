#pragma once

#include <deque>
#include "Editor/UndoRedo/BaseCommand.h"

class CommandManager
{
public:
	CommandManager(uint32_t uiMaxCommand = 50);
	~CommandManager();

	void Redo();
	void Undo();

	void AddCommand(BaseCommand* pCommand);
	void Clear();

private:
	std::deque<BaseCommand*> m_pCommands;
	uint32_t m_uiCurrentCommand = 0;
	uint32_t m_uiMaxCommand;
};