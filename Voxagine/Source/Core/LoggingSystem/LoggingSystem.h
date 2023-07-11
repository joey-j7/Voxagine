#pragma once

#include <vector>
#include <unordered_map>

#include "Core/LoggingSystem/LogEvent.h"
#include "Core/Event.h"

class Application;

class LoggingSystem
{
public:
	LoggingSystem();
	~LoggingSystem();

	void Initialize(Application* pApplication);
	void UnInitialize();

	void Log(const LogEvent& newLogEvent);
	void Log(const LogLevel& level, const std::string& description);
	void Log(const LogLevel& level, const std::string& category, const std::string& description);

	void CreateCategory(const std::string& newCategory);

	const LogEvent* GetLogEvent(unsigned long logEventIndex);
	unsigned long GetLogEventCount() const;
	const std::unordered_map<std::string, std::vector<unsigned long>>& GetCategories() { return m_Categories; }

	const std::vector<unsigned long>* GetEventLogCategoryIndices(const std::string& category);

public:
	Event<const LogEvent&, unsigned long> LogEventCreated;
	Event<const std::string> LogEventCategoryCreated;
private:
	Application* m_pApplication;

	std::vector<LogEvent*> m_LogEvents;
	std::unordered_map<std::string, std::vector<unsigned long>> m_Categories;
};