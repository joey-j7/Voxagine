#pragma once

#include "Editor/Window.h"

#include <unordered_map>
#include <string>

#include "Core/LoggingSystem/LogLevel.h"
#include "Editor/ConsoleLog/LogLevelLayout.h"

class RenderContext;

class LoggingSystem;
struct LogEvent;
class GameTimer;

class ConsoleLog : public Window
{
public:
	ConsoleLog();
	virtual ~ConsoleLog();

	virtual void Initialize(Editor* pTargetEditor);
	virtual void UnInitialize();

	virtual void OnContextResize(uint32_t a_uiWidth, uint32_t a_uiHeight, IVector2 deltaResolution) override;

	bool IsScrollToBottumEnabled() const;

protected:
	void OnPreRender(float fDeltaTime) override;
	void OnRender(float fDeltaTime) override;
	void OnPostRender(float fDeltaTime) override;

private:
	void RenderCustomToolBar();
	void RenderOutputLog();

	void SearchEventLog();
	void ClearSearchEventLog();

	void UpdateDisplay();
	void UpdateMouseScroll();
	void ResetOutputLineFocus();

	void EnableScrollToButtom();
	void DisableScrollToButtom();

	void OnLogEventOutput();
	void OnLogEventSearchOutput();

	void LogEventFiltersAddLogLevel(LogLevel level);
	void LogEventFiltersRemoveLogLevel(LogLevel Level);

	void LogEventFilterAddLogLevel(const std::vector<unsigned long>& logEventIndices);
	void LogEventFilterRemoveLogLevel(LogLevel level);

	void LogEventSearchFiltersAddLogLevel(const std::vector<unsigned long>& logEventIndices);
	void LogEventSearchFiltersRemoveLogLevel(LogLevel level);

	void LogEventFiltersAddCategory(const std::string& category);
	void LogEventFiltersRemoveCategory(const std::string& category);

	void LogEventFilterAddCategory(const std::vector<unsigned long>& logEventIndices);
	void LogEventFilterRemoveCategory(const std::string& category);

	void LogEventSearchFilterAddCategory(const std::vector<unsigned long>& logEventIndices);
	void LogEventSearchFilterRemoveCategory(const std::string& category);

	void OnLogEventFilterChange();

	void OnLogEventCreated(const LogEvent& event, unsigned long uiLogEventID);
	void OnLogEventCategoryCreated(const std::string newCategory);

private:
	LoggingSystem* m_pLog;
	std::unordered_map<LogLevel, LogLevelLayout> m_LogLevelLookUp;

	const GameTimer* m_pTimer = nullptr;
	RenderContext* m_pRenderContext = nullptr;

	std::unordered_map<std::string, bool> m_CategoryFilters;
	std::unordered_map<LogLevel, bool> m_LogLevelFilters;
	std::vector<unsigned long> m_LogEventFilter;

	std::vector<unsigned long> m_LogEventSearchFilter;
	std::string m_SearchString;
	std::string m_TempSearchString;
	bool m_bLogEventSearch;

	std::vector<unsigned long>* m_TargetLogEventFilter;

	bool m_bDisplayNeedsUpdate;
	bool m_bScrollToBottum;
	float m_fScrollValue;

	unsigned long m_ulDisplayLineCount;
	float m_fOutputLineFocus;

	unsigned m_uiWindowWidth;
	unsigned m_uiWindowHeight;
};