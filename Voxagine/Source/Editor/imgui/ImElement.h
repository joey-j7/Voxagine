#pragma once

#include "pch.h"

/**
 * @brief - Base class ImGui Element class.
 * Basic usage is to inherit from this class and then override the draw method
 */
struct ImElement 
{
	ImElement(std::string sTitle, int iFlags) : m_sTitle(sTitle), m_iFlags(iFlags) { }
	virtual ~ImElement() = default;

	/**
	 * @brief Implementation if we need to draw the object
	 *
	 * @return bool
	*/
	virtual void Draw() = 0;

protected:
	/**
	 * @param Title - The name of the element
	 */
	std::string m_sTitle = "";
	
	/**
	 * @param Present - boolean to show or hide the element
	 */
	bool bPresent = true;

	/**
	 * @param m_iFlags - ImGui flags for the property
	 * Keep it mind it uses 
	 */
	int m_iFlags = 0;
};