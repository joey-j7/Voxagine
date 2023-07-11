#include "pch.h"
#include "Editor/UndoRedo/CommandManager.h"

CommandManager::CommandManager(uint32_t uiMaxCommand)
	: m_uiMaxCommand(uiMaxCommand)
{
}

CommandManager::~CommandManager()
{
	for (auto && it : m_pCommands)
		delete it;
}

void CommandManager::Redo()
{
	if (m_uiCurrentCommand != m_pCommands.size())
	{
		++m_uiCurrentCommand;
		m_pCommands[m_uiCurrentCommand - 1]->Redo();
	}
}

void CommandManager::Undo()
{
	if (m_uiCurrentCommand != 0)
	{
		m_pCommands[m_uiCurrentCommand - 1]->Undo();
		--m_uiCurrentCommand;
	}
}

void CommandManager::AddCommand(BaseCommand * pCommand)
{
	if (m_uiCurrentCommand == m_uiMaxCommand)
	{
		delete m_pCommands.front();
		m_pCommands.pop_front();
	}
	else
	{
		if (m_uiCurrentCommand < m_pCommands.size())
		{
			while (m_pCommands.size() != m_uiCurrentCommand)
			{
				delete m_pCommands.back();
				m_pCommands.pop_back();
			}
		}

		++m_uiCurrentCommand;
	}

	m_pCommands.push_back(pCommand);
	pCommand->Execute();
}

void CommandManager::Clear()
{
	for (auto && it : m_pCommands)
		delete it;

	m_uiCurrentCommand = 0;
}
