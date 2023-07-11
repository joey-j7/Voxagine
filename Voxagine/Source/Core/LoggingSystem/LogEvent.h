#pragma once

#include "Core/Time.h"
#include "Core/LoggingSystem/LogLevel.h"

#include <string>

struct LogEvent
{
	Time EventTime;
	LogLevel Level;
	std::string Category;
	std::string Description;
};