#include "pch.h"
#include "Editor/ConsoleLog/ConsoleLog.h"

#include <algorithm>
#include <functional>
#include <string>

#include "Core/LoggingSystem/LoggingSystem.h"
#include "External/imgui/imgui.h"
#include "Editor/Editor.h"
#include "Core/Application.h"
#include "Core/Platform/Rendering/RenderContext.h"
#include <Core/Platform/Platform.h>
#include "Core/Platform/Input/Temp/InputContextNew.h"
#include "../EditorButton.h"

ConsoleLog::ConsoleLog()
	: m_pLog(nullptr)
	, m_LogLevelLookUp()
	, m_bScrollToBottum(true)
	, m_ulDisplayLineCount(10)
	, m_fOutputLineFocus(0.f)
	, m_uiWindowWidth(700)
	, m_uiWindowHeight(205)
	, m_bLogEventSearch(false)
	, m_TargetLogEventFilter(&m_LogEventFilter)
{
}

ConsoleLog::~ConsoleLog()
{
}

void ConsoleLog::Initialize(Editor * pTargetEditor)
{
	Window::Initialize(pTargetEditor);

	m_pTimer = &pTargetEditor->GetApplication()->GetTimer();
	m_pRenderContext = GetEditor()->GetApplication()->GetPlatform().GetRenderContext();

	// Set Window information first
	UVector2 renderRes = GetEditor()->GetApplication()->GetPlatform().GetRenderContext()->GetRenderResolution();

	SetPosition(ImVec2(290.0f, (float)renderRes.y - 250));
	SetSize(ImVec2((float)renderRes.x - 290.0f - 290.0f, 250));

	m_uiWindowWidth = static_cast<unsigned int>(GetSize().x - 10.f);
	m_uiWindowHeight = static_cast<unsigned int>(GetSize().y - 82.f);

	SetName("");

	SetWindowFlag(ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove /*| ImGuiWindowFlags_NoScrollbar*/);

	// Set the logging system pointer by value of the instance within application
	m_pLog = &GetEditor()->GetApplication()->GetLoggingSystem();

	// Enable already set categories
	for (auto& category : m_pLog->GetCategories())
	{
		m_CategoryFilters[category.first] = true;
	}

	// Register to the Log Event Category created event of logging system
	m_pLog->LogEventCategoryCreated += Event<const std::string>::Subscriber(std::bind(&ConsoleLog::OnLogEventCategoryCreated, this, std::placeholders::_1), this);

	// Register to the Log Event created event of logging system
	m_pLog->LogEventCreated += Event<const LogEvent&, unsigned long>::Subscriber(std::bind(&ConsoleLog::OnLogEventCreated, this, std::placeholders::_1, std::placeholders::_2), this);

	// Initialize the Log Level lookup table
	m_LogLevelLookUp[LogLevel::LOGLEVEL_MESSAGE] = LogLevelLayout{ "MESSAGE", ImVec4(0.752941f, 0.752941f, 0.752941f, 1.f) };
	m_LogLevelLookUp[LogLevel::LOGLEVEL_WARNING] = LogLevelLayout{ "WARNING", ImVec4(1.f, 1.f, 0.f, 1.f) };
	m_LogLevelLookUp[LogLevel::LOGLEVEL_ERROR] = LogLevelLayout{ "ERROR", ImVec4(1.f, 0.5f, 0.f, 1.f) };
	m_LogLevelLookUp[LogLevel::LOGLEVEL_CRITICAL_ERROR] = LogLevelLayout{ "CRASH", ImVec4(1.f, 0.f, 0.f, 1.f) };

	m_LogLevelFilters[LogLevel::LOGLEVEL_MESSAGE] = true;
	m_LogLevelFilters[LogLevel::LOGLEVEL_WARNING] = true;
	m_LogLevelFilters[LogLevel::LOGLEVEL_ERROR] = true;
	m_LogLevelFilters[LogLevel::LOGLEVEL_CRITICAL_ERROR] = true;
}

void ConsoleLog::UnInitialize()
{
	Window::UnInitialize();
}

void ConsoleLog::OnContextResize(uint32_t a_uiWidth, uint32_t a_uiHeight, IVector2 deltaResolution)
{
	SetSize(ImVec2((float)a_uiWidth - 290.0f - 290.0f, 250));
	SetPosition(ImVec2(290.0f, (float)a_uiHeight - 250));

	m_uiWindowWidth = static_cast<unsigned int>(GetSize().x - 10.f);
	m_uiWindowHeight = static_cast<unsigned int>(GetSize().y - 86.f);

	ImGui::SetWindowPos(GetName().data(), GetPosition());
	ImGui::SetWindowSize(GetName().data(), GetSize());
}

bool ConsoleLog::IsScrollToBottumEnabled() const
{
	return m_bScrollToBottum;
}

void ConsoleLog::OnPreRender(float fDeltaTime)
{
	Window::OnPreRender(fDeltaTime);

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(17.0f, 2.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.135f, 0.165f, 0.204f, 0.9f));
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.175f, 0.205f, 0.244f, 0.9f));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.11f, 0.12f, 0.15f, 0.9f));

	ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.f, 0.f, 0.f, 0.f));
	ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.f, 0.f, 0.f, 0.f));
	ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.f, 0.f, 0.f, 0.f));

}

void ConsoleLog::OnRender(float fDeltaTime)
{
	ImGui::PopStyleVar(1);

	/* Title */
	ImGui::SameLine(0.f);
	ImGui::Text("Console Log");

	/* FPS Counter */
	uint32_t uiGPUTime = m_pRenderContext->GetFPS();
	std::string fpsString = (std::to_string(uiGPUTime) + " FPS");

	if (!fpsString.empty())
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, -10.0f));

		ImVec2 textSize = ImGui::CalcTextSize(fpsString.c_str());

		ImGui::SameLine(m_uiWindowWidth - textSize.x - 7.0f);
		ImGui::Text(fpsString.c_str());

		ImGui::PopStyleVar();
	}

	RenderCustomToolBar();

	RenderOutputLog();

	ImGui::PopStyleVar();
	ImGui::PopStyleColor(6);
}

void ConsoleLog::OnPostRender(float fDeltaTime)
{
	Window::OnPostRender(fDeltaTime);
}

void ConsoleLog::RenderCustomToolBar()
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5.0f, 7.0f));

	// Combo for selecting category for filtering log events
	ImGui::PushItemWidth(150);

	if (ImGui::BeginCombo("##CategoryFilterCombo", "Category Filters"))
	{
		for (std::unordered_map<std::string, bool>::iterator it = m_CategoryFilters.begin(); it != m_CategoryFilters.end(); ++it)
		{
			bool TempSelected = it->second;
			ImGui::Checkbox(it->first.c_str(), &it->second);

			if (TempSelected != it->second)
				(it->second == true) ? LogEventFiltersAddCategory(it->first) : LogEventFiltersRemoveCategory(it->first);
		}

		ImGui::EndCombo();
	}

	ImGui::SameLine();
	ImGui::PushItemWidth(150);

	if (ImGui::BeginCombo("##LogLevelFilterCombo", "Filter Levels"))
	{
		for (std::unordered_map<LogLevel, bool>::iterator it = m_LogLevelFilters.begin(); it != m_LogLevelFilters.end(); ++it)
		{
			bool TempSelected = it->second;
			ImGui::Checkbox(m_LogLevelLookUp[it->first].LevelText.c_str(), &it->second);

			if (TempSelected != it->second)
				(it->second == true) ? LogEventFiltersAddLogLevel(it->first) : LogEventFiltersRemoveLogLevel(it->first);
		}

		ImGui::EndCombo();
	}

	// Input text box for the search functionality
	ImGui::SameLine();
	ImGui::PushItemWidth(static_cast<float>(m_uiWindowWidth) - 150.f - 150.f - 93.f);
	if (ImGui::InputText("###ConsoleLogSearch", (char*)m_TempSearchString.c_str(), m_TempSearchString.capacity(), ImGuiInputTextFlags_EnterReturnsTrue))
		SearchEventLog();

	ImGui::PopStyleVar();

	// Button for start search function
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));

	GetEditor()->GetButton("search_console")->OnClick([this] { SearchEventLog(); });
	
	// Button for erasing the search function
	ImGui::SameLine();
	GetEditor()->GetButton("erase_console")->OnClick([this] { ClearSearchEventLog(); });

	ImGui::PopStyleColor(3);
}

void ConsoleLog::RenderOutputLog()
{
	ImGui::BeginChild(
		"Console Log",
		ImVec2(
			static_cast<float>(m_uiWindowWidth),
			static_cast<float>(m_uiWindowHeight)
		),
		false,
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar //| ImGuiWindowFlags_NoScrollbar
	);

	// Update mouse scroll logic first
	UpdateMouseScroll();

	// Update/calculate console log display
	UpdateDisplay();

	unsigned DisplayStringStart, DisplayStringEnd;

	DisplayStringEnd = static_cast<unsigned int>((m_fOutputLineFocus < m_TargetLogEventFilter->size()) ? ceil(m_fOutputLineFocus) : m_TargetLogEventFilter->size());
	DisplayStringStart = static_cast<unsigned int>((m_fOutputLineFocus - m_ulDisplayLineCount <= 0.f) ? 0 : floor(m_fOutputLineFocus - m_ulDisplayLineCount));

	for (unsigned long it = DisplayStringStart; it != DisplayStringEnd; ++it)
	{
		const LogEvent*  Event = m_pLog->GetLogEvent((*m_TargetLogEventFilter)[it]);
		const LogLevelLayout& Layout = m_LogLevelLookUp[Event->Level];

		std::string OutputText;
		OutputText += "[" + std::to_string(Event->EventTime.Hours) + ":" + std::to_string(Event->EventTime.Minutes) + ":" + std::to_string(Event->EventTime.Seconds) + "] ";
		OutputText += "[" + Layout.LevelText + "] ";
		OutputText += "[" + Event->Category + "] ";
		OutputText += Event->Description;

		ImGui::PushTextWrapPos(-1.f);
		ImGui::TextColored(Layout.Color, OutputText.c_str());
		ImGui::PopTextWrapPos();
	}

	ImGui::EndChild();
}

void ConsoleLog::SearchEventLog()
{
	m_TempSearchString.assign(m_TempSearchString.data());
	m_SearchString.assign(m_TempSearchString.c_str());

	if (m_SearchString.empty())
	{
		ClearSearchEventLog();
		return;
	}

	m_LogEventSearchFilter.clear();

	for (std::vector<unsigned long>::iterator it = m_LogEventFilter.begin(); it != m_LogEventFilter.end(); ++it)
	{
		const LogEvent* CurEvent = m_pLog->GetLogEvent(*it);
		if (CurEvent->Description.find(m_SearchString.data()) != std::string::npos)
		{
			m_LogEventSearchFilter.push_back(*it);
		}
	}

	OnLogEventSearchOutput();
}

void ConsoleLog::ClearSearchEventLog()
{
	if (!m_TempSearchString.empty())
	{
		m_TempSearchString.clear();
		m_SearchString.clear();
		m_LogEventSearchFilter.clear();
		OnLogEventOutput();
	}
}

void ConsoleLog::UpdateDisplay()
{
	if (m_bDisplayNeedsUpdate)
	{
		m_fOutputLineFocus += m_fScrollValue;

		m_fOutputLineFocus = std::max((float)m_ulDisplayLineCount, std::min(m_fOutputLineFocus, (float)m_TargetLogEventFilter->size()));

		(m_fOutputLineFocus != (float)m_TargetLogEventFilter->size() && m_TargetLogEventFilter->size() > m_ulDisplayLineCount) ? DisableScrollToButtom() : EnableScrollToButtom();

		m_bDisplayNeedsUpdate = false;
		m_fScrollValue = 0.f;
	}
}

void ConsoleLog::UpdateMouseScroll()
{
	ImVec2 OutPutLogPos = ImGui::GetWindowPos();
	ImVec2 OutPutLogSize = ImGui::GetWindowSize();

	if (ImGui::IsMouseHoveringRect(OutPutLogPos, ImVec2(OutPutLogPos.x + OutPutLogSize.x, OutPutLogPos.y + OutPutLogSize.y)))
	{
		float ScrollDeltaTime = -(GetEditor()->GetApplication()->GetPlatform().GetInputContext()->GetMouseWheelDelta());

		if (ScrollDeltaTime != 0.f)
		{
			ScrollDeltaTime = (ScrollDeltaTime > 0.f) ? 1.f : -1.f;

			m_fScrollValue = ScrollDeltaTime;
			m_bDisplayNeedsUpdate = true;
		}
	}
}

void ConsoleLog::ResetOutputLineFocus()
{
	m_fOutputLineFocus = static_cast<float>(m_TargetLogEventFilter->size());
	m_bDisplayNeedsUpdate = true;
}

void ConsoleLog::EnableScrollToButtom()
{
	m_bScrollToBottum = true;
}

void ConsoleLog::DisableScrollToButtom()
{
	m_bScrollToBottum = false;
}

void ConsoleLog::OnLogEventOutput()
{
	m_bLogEventSearch = false;
	m_TargetLogEventFilter = &m_LogEventFilter;
	ResetOutputLineFocus();
}

void ConsoleLog::OnLogEventSearchOutput()
{
	m_bLogEventSearch = true;
	m_TargetLogEventFilter = &m_LogEventSearchFilter;
	ResetOutputLineFocus();
}

void ConsoleLog::LogEventFiltersAddLogLevel(LogLevel level)
{
	std::vector<const std::vector<unsigned long>*> LogEventCategories;
	unsigned long ReserveCount = 0;

	for (std::unordered_map<std::string, bool>::const_iterator it = m_CategoryFilters.begin(); it != m_CategoryFilters.end(); ++it)
	{
		if (it->second)
		{
			LogEventCategories.push_back(m_pLog->GetEventLogCategoryIndices(it->first));

			ReserveCount += static_cast<unsigned long>(LogEventCategories[LogEventCategories.size() - 1]->size());
		}
	}

	std::vector<unsigned long> LogLevelIndicesToAdd;
	LogLevelIndicesToAdd.reserve(ReserveCount);

	for (std::vector<const std::vector<unsigned long>*>::const_iterator itcat = LogEventCategories.begin(); itcat != LogEventCategories.end(); ++itcat)
	{
		for (std::vector<unsigned long>::const_iterator itind = (*itcat)->begin(); itind != (*itcat)->end(); ++itind)
		{
			if (m_pLog->GetLogEvent(*itind)->Level == level)
				LogLevelIndicesToAdd.push_back(*itind);
		}
	}

	// For improvement in future
	//std::sort(LogLevelIndicesToRemove.begin(), LogLevelIndicesToRemove.end());

	LogEventFilterAddLogLevel(LogLevelIndicesToAdd);

	if (m_bLogEventSearch)
		LogEventSearchFiltersAddLogLevel(LogLevelIndicesToAdd);

	OnLogEventFilterChange();
}

void ConsoleLog::LogEventFiltersRemoveLogLevel(LogLevel level)
{
	LogEventFilterRemoveLogLevel(level);

	if (m_bLogEventSearch)
		LogEventSearchFiltersRemoveLogLevel(level);

	OnLogEventFilterChange();
}

void ConsoleLog::LogEventFilterAddLogLevel(const std::vector<unsigned long>& logEventIndices)
{
	unsigned long NeededCap = static_cast<unsigned long>(m_LogEventFilter.size() + logEventIndices.size());

	if (m_LogEventFilter.capacity() < NeededCap)
		m_LogEventFilter.reserve(NeededCap);

	for (std::vector<unsigned long>::const_iterator it = logEventIndices.begin(); it != logEventIndices.end(); ++it)
	{
		m_LogEventFilter.push_back(*it);
	}

	std::sort(m_LogEventFilter.begin(), m_LogEventFilter.end());
}

void ConsoleLog::LogEventFilterRemoveLogLevel(LogLevel level)
{
	for (std::vector<unsigned long>::iterator it = m_LogEventFilter.begin(); it != m_LogEventFilter.end();)
	{
		if (m_pLog->GetLogEvent(*it)->Level == level)
			it = m_LogEventFilter.erase(it);
		else
			++it;
	}
}

void ConsoleLog::LogEventSearchFiltersAddLogLevel(const std::vector<unsigned long>& logEventIndices)
{
	unsigned long NeededCap = static_cast<unsigned long>(m_LogEventSearchFilter.size() + logEventIndices.size());

	if (m_LogEventSearchFilter.capacity() < NeededCap)
		m_LogEventSearchFilter.reserve(NeededCap);


	for (std::vector<unsigned long>::const_iterator it = logEventIndices.begin(); it != logEventIndices.end(); ++it)
	{
		if (m_pLog->GetLogEvent(*it)->Description.find(m_SearchString.data()) != std::string::npos)
			m_LogEventSearchFilter.push_back(*it);
	}

	std::sort(m_LogEventSearchFilter.begin(), m_LogEventSearchFilter.end());
}

void ConsoleLog::LogEventSearchFiltersRemoveLogLevel(LogLevel level)
{
	for (std::vector<unsigned long>::iterator it = m_LogEventSearchFilter.begin(); it != m_LogEventSearchFilter.end();)
	{
		if (m_pLog->GetLogEvent(*it)->Level == level)
			it = m_LogEventSearchFilter.erase(it);
		else
			++it;
	}
}

void ConsoleLog::LogEventFiltersAddCategory(const std::string & category)
{
	const std::vector<unsigned long>* LogEventIndices = m_pLog->GetEventLogCategoryIndices(category);
	std::vector<unsigned long> LogCategoryIndicesToAdd;

	LogLevel LogLevelFilter[4];
	unsigned LogLevelFilterCount = 0;

	for (std::unordered_map<LogLevel, bool>::const_iterator itl = m_LogLevelFilters.begin(); itl != m_LogLevelFilters.end(); ++itl)
	{
		if (itl->second)
		{
			LogLevelFilter[LogLevelFilterCount] = itl->first;
			++LogLevelFilterCount;
		}
	}

	for (std::vector<unsigned long>::const_iterator it = LogEventIndices->begin(); it != LogEventIndices->end(); ++it)
	{
		for (unsigned i = 0; i != LogLevelFilterCount; ++i)
		{
			if (m_pLog->GetLogEvent(*it)->Level == LogLevelFilter[i])
				LogCategoryIndicesToAdd.push_back(*it);
		}
	}

	// For improvement in future
	//std::sort(LogLevelIndicesToRemove.begin(), LogLevelIndicesToRemove.end());

	LogEventFilterAddCategory(LogCategoryIndicesToAdd);

	if (m_bLogEventSearch)
		LogEventSearchFilterAddCategory(LogCategoryIndicesToAdd);

	OnLogEventFilterChange();
}

void ConsoleLog::LogEventFiltersRemoveCategory(const std::string & category)
{
	LogEventFilterRemoveCategory(category);

	if (m_bLogEventSearch)
		LogEventSearchFilterRemoveCategory(category);

	OnLogEventFilterChange();
}

void ConsoleLog::LogEventFilterAddCategory(const std::vector<unsigned long>& logEventIndices)
{
	unsigned long NeededCap = static_cast<unsigned long>(m_LogEventFilter.size() + logEventIndices.size());

	if (m_LogEventFilter.capacity() < NeededCap)
		m_LogEventFilter.reserve(NeededCap);

	for (std::vector<unsigned long>::const_iterator it = logEventIndices.begin(); it != logEventIndices.end(); ++it)
		m_LogEventFilter.push_back(*it);

	std::sort(m_LogEventFilter.begin(), m_LogEventFilter.end());
}

void ConsoleLog::LogEventFilterRemoveCategory(const std::string & category)
{
	for (std::vector<unsigned long>::iterator it = m_LogEventFilter.begin(); it != m_LogEventFilter.end();)
	{
		if (m_pLog->GetLogEvent(*it)->Category == category)
			it = m_LogEventFilter.erase(it);
		else
			++it;
	}
}

void ConsoleLog::LogEventSearchFilterAddCategory(const std::vector<unsigned long>& logEventIndices)
{
	unsigned long NeededCap = static_cast<unsigned long>(m_LogEventSearchFilter.size() + logEventIndices.size());

	if (m_LogEventSearchFilter.capacity() < NeededCap)
		m_LogEventSearchFilter.reserve(NeededCap);


	for (std::vector<unsigned long>::const_iterator it = logEventIndices.begin(); it != logEventIndices.end(); ++it)
	{
		if (m_pLog->GetLogEvent(*it)->Description.find(m_SearchString.data()) != std::string::npos)
			m_LogEventSearchFilter.push_back(*it);
	}

	std::sort(m_LogEventSearchFilter.begin(), m_LogEventSearchFilter.end());
}

void ConsoleLog::LogEventSearchFilterRemoveCategory(const std::string & category)
{
	for (std::vector<unsigned long>::iterator it = m_LogEventSearchFilter.begin(); it != m_LogEventSearchFilter.end();)
	{
		if (m_pLog->GetLogEvent(*it)->Category == category)
			it = m_LogEventSearchFilter.erase(it);
		else
			++it;
	}
}

void ConsoleLog::OnLogEventFilterChange()
{
	m_fOutputLineFocus = static_cast<float>(m_TargetLogEventFilter->size());
	m_bDisplayNeedsUpdate = true;
}

void ConsoleLog::OnLogEventCreated(const LogEvent & event, unsigned long uiLogEventID)
{
	if (m_CategoryFilters[event.Category] && m_LogLevelFilters[event.Level])
	{
		m_LogEventFilter.push_back(uiLogEventID);

		if (m_bLogEventSearch)
		{
			if (event.Description.find(m_SearchString.data()) != std::string::npos)
			{
				m_LogEventSearchFilter.push_back(uiLogEventID);
			}
		}

		if (IsScrollToBottumEnabled())
		{
			m_fOutputLineFocus += 1;
			m_bDisplayNeedsUpdate = true;
		}
	}
}

void ConsoleLog::OnLogEventCategoryCreated(const std::string newCategory)
{
	m_CategoryFilters[newCategory] = true;
}
