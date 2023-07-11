#pragma once

#include "UIComponent.h"

#include <External/rttr/type>

#include <Core/Event.h>

enum class EOrientation {
	HORIZONTAL,
	VERTICAL
};

class SpriteRenderer;

class UISlider : public UIComponent
{
public:

	UISlider(Entity* pOwner);
	virtual ~UISlider();

	void Awake() override;
	void Start() override;

	void SetIsFocusable(bool); // Change to disabled state??!?

	void UpdateSliderRepresentation();
	void CreateInputBindings();
	void RemoveInputBindings();

	void UpdateValueChange(bool);
	void SetValue(float);
	void IncrementValue();
	void DecrementValue();
	float GetValue() const;

	void SetStepSize(float);
	float GetStepSize() const;

	void SetStepInterval(float);
	float GetStepInterval() const;
	void SetStepIntervalMS(float); 
	float GetStepIntervalMS() const;

	void SetOrientation(EOrientation);
	EOrientation GetOrientation() const;
	void SetInverted(bool);
	bool GetInverted() const;

	void SetIsProgress(bool);
	bool IsProgressBar() const;

protected:

	void OnFocus() override;
	void OnFocusLost() override;

private:
	void SetState(EUIState);

public:

	Event<UISlider*, float> m_ValueChangedEvent;

	Event<UISlider*> m_FocusEvent;
	Event<UISlider*> m_LostFocusEvent;


	SpriteRenderer* m_SliderBackground = nullptr;
	SpriteRenderer* m_SliderBar = nullptr;
	Entity* m_SliderHandleDefault = nullptr;
	Entity* m_SliderHandleFocused = nullptr;
	Entity* m_SliderHandleDisabled = nullptr;

private:

	float m_PreviousTime = 0;
	float m_StepInterval;

	bool m_bProgressBar;

	float m_Value;
	float m_StepSize;

	EOrientation m_Orientation;
	bool m_bInverted;

	std::vector<uint64_t> m_InputBindings;

	RTTR_REGISTRATION_FRIEND
	RTTR_ENABLE(UIComponent)
};