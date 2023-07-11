#include "pch.h"
#include "InputBindingBase.h"

uint64_t InputBindingBase::UNIQUE_INPUT_HANDLE_COUNT = 0;

uint64_t InputBindingBase::GenerateUniqueHandleID()
{
	uint64_t newUniqueHandle = UNIQUE_INPUT_HANDLE_COUNT;
	++UNIQUE_INPUT_HANDLE_COUNT;
	return newUniqueHandle;
}
