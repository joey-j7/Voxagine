#pragma once

#include "Editor/Window.h"

#include <Core/Event.h>

class ConfigurationWindow : public Window
{
public:
	ConfigurationWindow();
	virtual ~ConfigurationWindow();

	virtual void Initialize(Editor* pTargetEditor) override;
	virtual void UnInitialize() override;

	virtual void OnContextResize(uint32_t a_uiWidth, uint32_t a_uiHeight, IVector2 deltaResolution) {};

	Event<> OnWindowClose;

protected:
	void ResizeWindow();

protected:
	void CloseWindow();
};