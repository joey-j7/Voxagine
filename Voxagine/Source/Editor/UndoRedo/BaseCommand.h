#pragma once

class CommandManager;

class BaseCommand
{
	friend class CommandManager;
public:
	BaseCommand() {};
	virtual ~BaseCommand() {};

	virtual void Redo() = 0;
	virtual void Undo() = 0;

private:
	void Execute() { Redo(); };
};