#pragma once
#include <map>
#include "Core/Platform/Input/InputContext.h"

#include <wrl/wrappers/corewrappers.h>

#include <External/DirectXTK12/GamePad.h>
#include <External/DirectXTK12/Keyboard.h>
#include <External/DirectXTK12/Mouse.h>

class Platform;

using namespace DirectX;

class WINInputContext : public InputContext {
public:
	struct ActionState
	{
		bool bCurrentState;
		bool bPreviousState;
	};

	WINInputContext(Platform* pPlatform) : InputContext(pPlatform) {}
	~WINInputContext();

	virtual void Initialize() override;
	virtual void Update() override;

	GamePad* GetGamepad() { return m_pGamepad; }
	Keyboard* GetKeyboard() { return m_pKeyboard; }
	Mouse* GetMouse() { return m_pMouse; }

private:
	/* Input handling functions */
	void UpdateGamePadActionTable(DirectX::GamePad::State& gamePad);
	void UpdateGamePadAxisTable(DirectX::GamePad::State& gamePad);
	void UpdateMouseTable(DirectX::Mouse::State& mouse);
	void UpdateAxis();
	void UpdateActions();

	GamePad* m_pGamepad;
	Keyboard* m_pKeyboard;
	Mouse* m_pMouse;

	UVector2 m_prevMousePosition;

	GamePad::ButtonStateTracker m_GamePadButtons;
	Keyboard::KeyboardStateTracker m_KeyboardButtons;

	/* Input tables for binding InputBinding to corresponding GamePad and Keyboard states */
	std::map<InputBinding, uint32_t> m_KeyTable;
	std::map<InputBinding, float> m_GamePadAxisTable;
	std::map<InputBinding, float> m_MouseAxisTable;
	std::map<InputBinding, ActionState> m_MouseActionTable;
	std::map<InputBinding, ActionState> m_GamePadActionTable;
};