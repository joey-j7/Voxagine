#include "pch.h"
#include "Core/ECS/Components/InputHandler.h"

#include "Core/Application.h"
#include "Core/ECS/World.h"

#include <External/rttr/registration>
#include "Core/MetaData/PropertyTypeMetaData.h"

#include "External/DirectXTK12/GamePad.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<InputHandler>("InputHandler")
			.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
			.property("Player Handle", &InputHandler::GetPlayerHandle, &InputHandler::SetPlayerHandle) (RTTR_PUBLIC);
}

InputHandler::InputHandler(Entity * pEntity) :
	Component(pEntity)
{
	if (GetWorld() != nullptr)
		m_pInputContext = GetWorld()->GetApplication()->GetPlatform().GetInputContext();
}

InputHandler::~InputHandler()
{
	if (GetPlayerHandle() != INVALID_PLAYER_ID)
	{
		for (ActionBindingInformation& actionBindingInfoIt : m_ActionBindings)
			m_pPlayerController->UnBindAction(actionBindingInfoIt.ActionBindingHandle);

		for (uint64_t& handle : m_AxisHandles)
			m_pPlayerController->UnBindAxis(handle);

		m_pPlayerController = nullptr;
	}
}

void InputHandler::Awake()
{
	Component::Awake();

	if (!m_pInputContext)
		m_pInputContext = GetWorld()->GetApplication()->GetPlatform().GetInputContext();

	if (GetPlayerHandle() != INVALID_PLAYER_ID)
		m_pPlayerController = m_pInputContext->ReceivePlayerController(GetPlayerHandle());
}

void InputHandler::BindAxis(const std::string & layerName, const std::string & axisName, std::function<void(float)> bindFunc)
{
	if (GetPlayerHandle() != INVALID_PLAYER_ID)
	{
		uint64_t handle = m_pPlayerController->BindAxis(layerName, axisName, [this, bindFunc](float input) {
			if (IsEnabled())
				bindFunc(input);
		}, &GetWorld()->GetApplication()->GetWorldManager());

		m_AxisHandles.push_back(handle);
	}
}

void InputHandler::BindAction(const std::string& actionName, InputKeyStatus inputEvent, std::function<void()> bindFunc)
{
	BindAction(DEFAULT_INPUT_MAP_NAME, actionName, inputEvent, bindFunc);
}

void InputHandler::BindAction(const std::string & layerName, const std::string & actionName, InputKeyStatus inputEvent, std::function<void()> bindFunc)
{
	uint64_t bindHandle;
	BindAction(layerName, actionName, inputEvent, bindFunc, bindHandle);

	m_ActionBindings.push_back({
		bindHandle,
		layerName,
		actionName,
		inputEvent,
		bindFunc
	});
}

float InputHandler::GetAxisValue(const std::string & axisName)
{
	return (GetPlayerHandle() != INVALID_PLAYER_ID) ? m_pPlayerController->GetAxisValue(axisName).Value : 0.0f;
}

void InputHandler::SetPlayerHandle(int playerHandle)
{
	if (playerHandle < 0)
		playerHandle = 0;
	
	if (playerHandle > m_pInputContext->GetMaxPlayerCount())
		playerHandle = m_pInputContext->GetMaxPlayerCount();

	SwapPlayerHandle(playerHandle);
}

int InputHandler::GetPlayerHandle() const
{
	return m_iPlayerHandle;
}

void InputHandler::VibrateGamePad(float fLeftMotor, float fRightMotor)
{
	if (GetPlayerHandle() != INVALID_PLAYER_ID)
		m_pPlayerController->VibrateGamePad(fLeftMotor, fRightMotor);
}

MouseController* InputHandler::GetMouse()
{
	return (GetPlayerHandle() != INVALID_PLAYER_ID) ? m_pPlayerController->GetMouseController() : nullptr;
}

KeyboardController* InputHandler::GetKeyboard()
{
	return (GetPlayerHandle() != INVALID_PLAYER_ID) ? m_pPlayerController->GetKeyBoardController() : nullptr;
}

void InputHandler::SwapPlayerHandle(int newPlayerHandle)
{
	PlayerController* pOldPlayerController = m_pPlayerController;
	m_iPlayerHandle = newPlayerHandle;
	m_pPlayerController = m_pInputContext->ReceivePlayerController(GetPlayerHandle());

	for (unsigned int actionBindingInformationIt = 0; actionBindingInformationIt != m_ActionBindings.size(); ++actionBindingInformationIt)
	{
		ActionBindingInformation& actionBinding = m_ActionBindings[actionBindingInformationIt];

		if (pOldPlayerController != nullptr && actionBinding.ActionBindingHandle != INVALID_UNIQUE_INPUT_HANDLE)
			pOldPlayerController->UnBindAction(actionBinding.ActionBindingHandle);

		BindAction(actionBinding.LayerName, actionBinding.ActionName, actionBinding.InputKeyEvent, actionBinding.ActionBindingCallback, actionBinding.ActionBindingHandle);
	}
}

void InputHandler::BindAction(const std::string & layerName, const std::string & actionName, InputKeyStatus inputEvent, std::function<void()> bindFunc, uint64_t& newBindHandle)
{
	if (GetPlayerHandle() != INVALID_PLAYER_ID)
	{
		uint64_t handle = m_pPlayerController->BindAction(layerName, actionName, inputEvent, [this, bindFunc]() {
			if (IsEnabled())
				bindFunc();
		}, &GetWorld()->GetApplication()->GetWorldManager());

		newBindHandle = handle;
		return;
	}

	newBindHandle = INVALID_UNIQUE_INPUT_HANDLE;
}
