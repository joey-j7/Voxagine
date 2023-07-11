#pragma once

#include "InputBindingMap.h"
#include "InputBindingMapInterface.h"
#include "InputBindingHandlerInterface.h"

#define INVALID_PLAYER_ID 0

class InputContextNew;
class InputHandler;

class KeyboardController;
class MouseController;
class GamePadController;

class PlayerController : 
	protected InputBindingHandlerInterface, public InputBindingMapInterface
{
	friend class InputContextNew;
	friend class InputHandler;
public:
	PlayerController();
	~PlayerController();

	void Update();

	void VibrateGamePad(float fLeftMotor, float fRightMotor);

	bool CreateBindingMap(const std::string& bindingMapName, bool bSetActiveMap = true);
	bool DestroyBindingMap(const std::string& bindingMapName);
	const InputBindingMapInformation* GetBindingMap(const std::string& bindingMapName);

	bool SetActiveBindingMap(const std::string& bindingMapName);
	const InputBindingMapInformation* GetActiveBindingMap() const;

	void SetDefaultMapName(const std::string& defaultMapName);
	std::string GetDefaultMapName() const;

	void Possess() { m_IsPossessed = true; };
	void UnPossess() { m_IsPossessed = false; };
	bool IsPossessed() const { return m_IsPossessed; };
	int GetPlayerID() { return m_PlayerID; };

	InputBindingAxisValue GetAxisValue(const std::string& axisName);
	InputBindingAxisValue GetAxisValue(const std::string& bindingMapName, const std::string& axisName);

protected:
	void SetPlayerKeyboard(KeyboardController* pKeyboard) { m_pPlayerKeyboard = pKeyboard; };
	void SetPlayerMouse(MouseController* pMouse) { m_pPlayerMouse = pMouse; };
	void SetPlayerGamepad(GamePadController* pGamePad) { m_pPlayerGamePad = pGamePad; };

private:
	void ProcessInputBindings();

	InputBindingMap* GetInputBindingMap() override;
	MouseController* GetMouseController() override;
	KeyboardController* GetKeyBoardController() override;
	GamePadController* GetGamePadController() override;

private:
	static int UNIQUE_PLAYER_ID;

	InputBindingMap m_InputBindingMap;
	int m_PlayerID = INVALID_PLAYER_ID;
	bool m_IsPossessed = false;

	MouseController* m_pPlayerMouse = nullptr;
	KeyboardController* m_pPlayerKeyboard = nullptr;
	GamePadController* m_pPlayerGamePad = nullptr;
};