#pragma once

#include "InputKeyIdentifiers.h"
#include "InputBindingAxisValue.h"
#include <unordered_map>

class WindowContext;

class InputController
{
public:
	InputController();
	virtual ~InputController();

	virtual void Initialize(WindowContext* pWindowContext);
	virtual void UnInitialize();
	virtual void Update();

	bool IsConnected() const;

	bool IsKeyPressed(InputKey inputKey) const;
	bool IsKeyDown(InputKey inputKey) const;
	bool IsKeyReleased(InputKey inputKey) const;
	InputKeyStatus GetKeyStatus(InputKey inputKey) const;
	bool HasKey(InputKey inputKey) const;

	bool IsKeyAxisDown(InputKey inputKey) const;
	virtual InputBindingAxisValue GetAxisValue(InputKey inputKey) const;

protected:
	void AddInputKeyStateMap(InputKey inputKey, InputKeyStatus inputKeyStatus = InputKeyStatus::IKS_NONE);
	void AddInputKeyAxisMap(InputKey inputKey, float fAxisStatus = 0.f);
	void UpdateKeyState(InputKey inputKey, bool bKeyPressed);
	void UpdateAxisValue(InputKey inputKey, float fAxisValue);

	void SetConnected(bool bConnected);
private:
	virtual void OnInitialize() = 0;
	virtual void OnUninitialize() = 0;
	virtual void OnUpdate() = 0;

	virtual void InitializeButtons() {}; // Does nothing by default
	virtual void InitializeAxises() {}; // Does nothing by default
protected:
	// Controller input key mapping for states
	std::unordered_map<InputKey, InputKeyStatus> m_Buttons;
	// Controller input key mapping for axises
	std::unordered_map<InputKey, float> m_Axises;

	WindowContext* GetWindowContext();
private:
	WindowContext* m_pInputContext = nullptr;
	bool m_bConnected = false;
};