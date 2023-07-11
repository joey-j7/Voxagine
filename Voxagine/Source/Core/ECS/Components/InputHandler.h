#pragma once
#include "Core/ECS/Component.h"

#include "Core/Platform/Input/Temp/InputContextNew.h"
#include "Core/Platform/Input/Temp/InputKeyIdentifiers.h"
#include "Core/Platform/Input/Temp/MouseControllerInterface.h"
#include "Core/Platform/Input/Temp/KeyboardControllerInterface.h"

#include <External/rttr/type>

class InputContextNew;
class PlayerController;

class InputHandler : public Component 
	, public MouseControllerInterface , public KeyboardControllerInterface
{
private:
	struct ActionBindingInformation
	{
		uint64_t ActionBindingHandle;

		std::string LayerName;
		std::string ActionName;

		InputKeyStatus InputKeyEvent;
		std::function<void()> ActionBindingCallback;
	};

public:
	InputHandler(Entity* pEntity);
	virtual ~InputHandler();

	virtual void Awake() override;

	void BindAxis(const std::string& layerName, const std::string& axisName, std::function<void(float)> bindFunc);
	void BindAction(const std::string& actionName, InputKeyStatus inputEvent, std::function<void()> bindFunc);
	void BindAction(const std::string& layerName, const std::string& actionName, InputKeyStatus inputEvent, std::function<void()> bindFunc);

	float GetAxisValue(const std::string& axisName);

	void SetPlayerHandle(int playerHandle);
	int GetPlayerHandle() const;

	void VibrateGamePad(float fLeftMotor, float fRightMotor);

private:
	MouseController* GetMouse() override;
	KeyboardController* GetKeyboard() override;

	void SwapPlayerHandle(int newPlayerHandle);

	void BindAction(const std::string& layerName, const std::string& actionName, InputKeyStatus inputEvent, std::function<void()> bindFunc, uint64_t& newBindHandle);

private:
	InputContextNew* m_pInputContext;
	PlayerController* m_pPlayerController;
	int m_iPlayerHandle = INVALID_PLAYER_ID;

	std::vector<ActionBindingInformation> m_ActionBindings;
	std::vector<uint64_t> m_AxisHandles;

	RTTR_ENABLE(Component)
};