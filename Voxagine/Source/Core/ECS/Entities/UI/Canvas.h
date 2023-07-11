#pragma once

#include "Core/ECS/Entity.h"
#include "Core/Math.h"

#include <External/rttr/type>

#include <vector>

class Platform;
class UIComponent;
class InputContextNew;

#define UI_INPUT_LAYER "UIInput"

/*!
 * Canvas entity, used as a container for all UI entities and components in the world.
 * 
 */
class Canvas : public Entity
{
	friend class Player;
public:
	Canvas(World* pWorld);
	virtual ~Canvas();

	virtual void Awake() override;
	virtual void Start() override;

	virtual void Tick(float) override;

	void RegisterUIComponent(UIComponent*);
	void RemoveUIComponent(UIComponent*);

	virtual void OnEnabled() override;
	virtual void OnDisabled() override;

	virtual void SetNavigatable(bool);
	bool IsNavigatable() const;

protected:

	virtual void SetFocusPrevious();
	virtual void SetFocusNext();

	virtual void SetFocusUp();
	virtual void SetFocusDown();
	virtual void SetFocusLeft();
	virtual void SetFocusRight();


	void ChangeFocus(UIComponent*);
	UIComponent* GetDefaultFocus();


	virtual void OnPressed();
	virtual void OnPressedRepeat();
	virtual void OnReleased();

protected:

	std::vector<uint64_t> m_InputBindings;

private:

	// All registered UI entities
	std::vector<UIComponent*> m_UIComponents;

	bool m_bNavigatable;

	UIComponent* m_pFocusedComp = nullptr;
	UIComponent* m_pPressedComp = nullptr;

	std::string m_PreviousBindingMap;

	// Platform pointer used for data in the UIComponents.
	Platform* m_Platform = nullptr;
	InputContextNew* m_pInputContext = nullptr;

	Vector2 m_PrevInputValue;

	RTTR_ENABLE(Entity);
};