#pragma once

#include "InputController.h"

class GamePadController : public InputController
{
public:
	GamePadController();
	virtual ~GamePadController();

	virtual void Initialize(WindowContext* pWindowContext) override;

	void Vibrate(float fLeftMotor, float fRightMotor);

	int GetGamePadID() const;

private:
	void OnInitialize() override;
	void OnUninitialize() override;
	void OnUpdate() override;

	void InitializeButtons() override;
	void InitializeAxises() override;

	float NormalizeFloat(float fMax, float fValue);
private:
	// Static boolean indicating library dependencies being initialized
	static bool m_bInitLibraries;
	// Static int indicating the next unique ID for a gamepad controller
	static int m_iNextUniqGamePadID;
	// Static int indicating the amount of gamepad controllers active
	static int m_iGamePadControllerCount;

	// Gamepad controller information
	int32_t m_iGamePadHandle;
	int32_t m_iUniqueGamePadID;

	bool m_bVibrateIsDirty = false;
	float m_fVibrateLeftMotorValue = 0.f;
	float m_fVibrateRightMotorValue = 0.f;
};