#pragma once

#include "UIComponent.h"

#include <External/rttr/type>

#include <Core/Event.h>

class AudioSource;

class UIButton : public UIComponent
{
public:

	UIButton(Entity* pOwner);
	virtual ~UIButton();

	virtual void Awake() override;
	virtual void Start() override;

	void SetIsFocusable(bool);

protected:

	void OnFocus() override;
	void OnFocusLost() override;
	void OnPressed() override;
	void OnReleased() override;
	void OnClicked() override;

private:
	void SetState(EUIState);

public:
	Event<UIButton*> m_ClickedEvent;
	Event<UIButton*> m_PressedEvent;
	Event<UIButton*> m_ReleasedEvent;
	Event<UIButton*> m_FocusEvent;
	Event<UIButton*> m_LostFocusEvent;

	AudioSource* m_pAudioSource = nullptr;

	Entity* m_NormalObject = nullptr;
	Entity* m_FocusedObject = nullptr;
	Entity* m_PressedObject = nullptr;
	Entity* m_DisabledObject = nullptr;

	RTTR_REGISTRATION_FRIEND
	RTTR_ENABLE(UIComponent)
};