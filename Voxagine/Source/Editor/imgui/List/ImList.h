#pragma once

#include "Editor/imgui/ImElement.h"

#include <External/imgui/imgui_dropdown.h>
#include <External/imgui/imgui_stl.h>

/**
 * @brief ListType - The way how we should render the list
 */
enum ListType : int32_t
{
	LT_NORMAL,
	LT_DROPDOWN,
};

/**
 * @brief - List element with a index, title and value of the typename
 * @param uiIndex - Index of the element
 * @param sTitle - Title of the element
 * @param aValue - Value of the element
 */
template<typename Args>
struct ListElement
{
	uint32_t uiIndex = 0;
	std::string sTitle = "";
	Args aValue = Args();
};

/**
 * @brief - Callback function for when an element has been chosen.
 * @param sTitle title of the element
 * @param aValue value of the element
 */
template<typename ListTypeArgs>
using ListTypeArgsCallback = std::function<void(const std::string&, const ListTypeArgs&)>;

/**
 * @brief - ImList With this you can show a generic list
 * with ImGui where you can feed it information, do something for
 * each element and provide a function when an element is chosen
 */
template<typename ListTypeArgs>
class ImList : public ImElement
{
public:
	/**
	 * @brief - Initialize without any element inside the list.
	 * @param sTitle - Title of the list
	 * @param iFlags - ImGui flags on the BeginChild()
	 */
	ImList(std::string sTitle, int iFlags = 0);

	/**
	 * @brief - Initialize with a list pre made.
	 * @param sTitle - Title of the list
	 * @param vList - List of element with the ListElement with in there the index, title, value
	 * @param iFlags - ImGui flags on the BeginChild()
	 */
	ImList(std::string sTitle, std::vector<ListElement<ListTypeArgs>> vList, int iFlags = 0);

	/**
	 * @brief - Initialize with an initializer list.
	 * @param sTitle - Title of the list
	 * @param Initializer_list - List of element with the ListElement with in there the index, title, value
	 * but for this you can provide by yourself how much there should be in there from the start.
	 * @param iFlags - ImGui flags on the BeginChild()
	 */
	ImList(std::string sTitle, std::initializer_list<ListElement<ListTypeArgs>> Initializer_list, int iFlags = 0);

	/**
	 * @brief - Initialize with an initializer list.
	 * @param sTitle - Title of the list
	 * @param vListValues - List of values to insert in our bucket.
	 * @param ValueHandler - A callback to handle the value type in order to get the title.
	 * @param iFlags - ImGui flags on the BeginChild()
	 */
	ImList(std::string sTitle, const std::vector<ListTypeArgs>& vListValues, std::function<std::string(uint32_t uiCurrentIndex, const ListTypeArgs&)> ValueHandler, int iFlags = 0);

	/**
	 * @brief - Get current counter of the list
	 *
	 * @return uint32_t counter
	*/
	uint32_t GetCurrentCounter() const { return m_uiCounter; }

	/**
	 * @brief - Check if the list is filled or not.
	 *
	 * @return bool - true for empty or else false;
	 */
	bool Empty() const { return m_vListValues.empty(); }

	/**
	 * @brief - Set the current value by index
	 * @param uiArgument - argument to set it from.
	 */
	void SetCurrentValue(uint32_t uiArgument)
	{
		if (!Empty() && uiArgument >= 0)
		{
			for (ListElement<ListTypeArgs>& ListValue : m_vListValues)
			{
				if (ListValue.uiIndex == uiArgument)
				{
					m_CurrentValue = &ListValue;
					break;
				}
			}
		}
	}

	/**
	 * @brief - Retrieve the search string before it get "de-allocated"
	 * THIS IS VERY IMPORTED TO GRAB, unless you have a class that is valid each frame.
	 * When your variable goes out of scope and you want to keep track of the string use this method!
	 *
	 * @return std::string SearchQuery
	 */
	std::string GetSearchValue() const { return m_sListSearchString; }

	/**
	 * @brief - Set the value of the search string.
	 * @param sValue - New value of the search.
	 */
	void SetSearchValue(const char* sValue) { m_sListSearchString = sValue; }

	/**
	 * @brief - change the way of rendering the list
	 * @param eListType
	 */
	void SetRenderType(ListType eListType) { m_eListType = eListType; }

	/**
	 * @brief render the ImGui element
	 */
	void Draw() override;

	/**
	 * @brief - Add a function for when we click on a list value.
	 * @param Callback - Set the function of what should happen when you click on a value
	 */
	void AddListener(const ListTypeArgsCallback<ListTypeArgs>& Callback)
	{
		m_Callback = Callback;
	}

	/**
	 * @brief - Add a function to handle each element in the list.
	 * This function is if you want to do custom things with each element.
	 * @param Callback - Set the callback what should happen on each element.
	 */
	void AddHandler(const std::function<std::string(const std::string&)>& Callback)
	{
		m_ElementHandler = Callback;
	}

	/**
	 * @brief add new value to the list. You don't have to provide an index.
	 * @param sTitle - Title of the element to insert.
	 * @param Argument - Value of the element to insert.
	*/
	void AddValue(std::string sTitle, ListTypeArgs Argument)
	{
		AddValue(ListElement<ListTypeArgs>({ m_uiCounter++, sTitle, Argument }));
	}

	/**
	 * @param v2Size - Size of the list
	*/
	Vector2 v2Size = Vector2(0.0f);
protected:

	/**
	 * @brief add new value to the list. You don't have to provide an index.
	 * @param Argument - value to insert.
	*/
	void AddValue(ListElement<ListTypeArgs> Argument)
	{
		m_vListValues.emplace_back(Argument);
	}

	// Current value
	ListElement<ListTypeArgs>* m_CurrentValue = nullptr;

	// Callback
	ListTypeArgsCallback<ListTypeArgs> m_Callback = nullptr;
	std::function<std::string(const std::string&)> m_ElementHandler = nullptr;

	// Values and counter
	std::vector<ListElement<ListTypeArgs>> m_vListValues;
	uint32_t m_uiCounter = 0;

	/**
	 * @param sListSearchString - Search string for the list.
	 */
	std::string m_sListSearchString;

	/**
	 * @param m_eListType - Way we should render the list.
	 */
	ListType m_eListType = LT_NORMAL;
};

template<typename ListTypeArgs>
ImList<ListTypeArgs>::ImList(std::string sTitle, int iFlags) : ImElement(sTitle, iFlags)
{
	m_vListValues = { };
}

template <typename ListTypeArgs>
ImList<ListTypeArgs>::ImList(std::string sTitle, std::vector<ListElement<ListTypeArgs>> vList, int iFlags) : ImElement(sTitle, iFlags)
{
	m_vListValues.insert(m_vListValues.end(), make_move_iterator(vList.begin()), make_move_iterator(vList.end()));
}

template<typename ListTypeArgs>
ImList<ListTypeArgs>::ImList(std::string sTitle, std::initializer_list<ListElement<ListTypeArgs>> Initializer_list, int iFlags) : ImElement(sTitle, iFlags)
{
	m_vListValues.insert(m_vListValues.end(), make_move_iterator(Initializer_list.begin()), make_move_iterator(Initializer_list.end()));
}

template <typename ListTypeArgs>
ImList<ListTypeArgs>::ImList(std::string sTitle, const std::vector<ListTypeArgs>& vListValues, std::function<std::string(uint32_t, const ListTypeArgs&)> ValueHandler, int iFlags) : ImElement(sTitle, iFlags)
{
	for (const auto& ListValue : vListValues) m_vListValues.push_back({ m_uiCounter++, ValueHandler(m_uiCounter - 1, ListValue), ListValue });
}

template <typename ListTypeArgs>
void ImList<ListTypeArgs>::Draw()
{
	// This bool is needed to see if the dropdown is clicked.
	bool bOpen = false;

	// Just the rect size
	const ImVec2 SelectableSize = ImGui::GetItemRectSize();

	// Draw the dropdown
	if (m_eListType == LT_DROPDOWN)
		bOpen = ImGui::BeginButtonDropDown(("###" + m_sTitle + "Selection").c_str(), (m_CurrentValue) ? m_CurrentValue->sTitle.c_str() : nullptr);

	std::string SearchStringUpperCase = m_sListSearchString.c_str();
	// Draw the list
	if (m_eListType == LT_NORMAL || bOpen)
	{
		if (!bOpen)
		{
			// Make all characters uppercase
			std::transform(m_sListSearchString.begin(), m_sListSearchString.end(), SearchStringUpperCase.begin(), toupper);

			// Generate search bar
			ImGui::PushItemWidth(ImGui::GetWindowContentRegionWidth());
			ImGui::InputText(("###" + m_sTitle + "Search").c_str(), &m_sListSearchString);
			ImGui::PopItemWidth();

			ImGui::BeginChild((m_sTitle + "Selection").c_str(), ImVec2(v2Size.x, v2Size.y), false, m_iFlags);
		}

		for (ListElement<ListTypeArgs>& it : m_vListValues)
		{
			std::string TempResourceFileName = (m_ElementHandler) ? m_ElementHandler(it.sTitle) : it.sTitle;
			std::string ResourceFileNameUpperCase = TempResourceFileName;

			// Make all characters uppercase
			std::transform(TempResourceFileName.begin(), TempResourceFileName.end(), ResourceFileNameUpperCase.begin(), toupper);
			if (ResourceFileNameUpperCase.find(SearchStringUpperCase) != std::string::npos || m_sListSearchString.empty())
			{
				ImGui::PushItemWidth(SelectableSize.x);

				if (ImGui::Selectable(TempResourceFileName.data()))
				{
					m_CurrentValue = &it;
					// Call the created function to set the new value.
					if (m_Callback) m_Callback(it.sTitle, it.aValue);
				}

				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::Text("%s", it.sTitle.data());
					ImGui::EndTooltip();
				}
			}
		}
	}

	if (m_eListType == LT_NORMAL)
		ImGui::EndChild();

	if (m_eListType == LT_DROPDOWN && bOpen)
		ImGui::EndButtonDropDown();
}
