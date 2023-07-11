#pragma once

#include "Editor/Window.h"

class EntityWizard : public Window
{
public:
	EntityWizard();
	~EntityWizard();

	virtual void Initialize(Editor* pTargetEditor) override;
	virtual void UnInitialize() override;

	virtual void Tick(float fDeltaTime) override;
// 	virtual void Render(float fDeltaTime) override;
// 	virtual void EndRendering(float fDeltaTime) override;

	virtual void OnContextResize(uint32_t a_uiWidth, uint32_t a_uiHeight, IVector2 deltaResolution) override;
};