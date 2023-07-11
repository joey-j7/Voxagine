#include "pch.h"
#include "CommandEngine.h"

PCommandEngine* CommandEngine::Get()
{
	return reinterpret_cast<PCommandEngine*>(this);
}