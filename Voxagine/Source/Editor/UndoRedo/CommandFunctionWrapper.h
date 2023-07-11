#pragma once

class CommandFunctionWrapper
{
public:
	CommandFunctionWrapper() {};
	virtual ~CommandFunctionWrapper() {};

	virtual void ExecuteFunction() = 0;
};