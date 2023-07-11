#pragma once

#include <stdint.h>
#include <string>

#define INVALID_UNIQUE_INPUT_HANDLE 18446744073709551615

class InputBindingBase
{
public:
	InputBindingBase(std::string bindingName) : m_BindingName(bindingName){};
	virtual ~InputBindingBase() {};

	const std::string& GetName() const { return m_BindingName; };
protected:
	uint64_t GenerateUniqueHandleID();
private:
	static uint64_t UNIQUE_INPUT_HANDLE_COUNT;
	std::string m_BindingName;
};