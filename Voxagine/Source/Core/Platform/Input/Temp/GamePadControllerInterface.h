#pragma once

class GamePadController;

class GamePadControllerInterface
{
public:
	GamePadControllerInterface();
	virtual ~GamePadControllerInterface();

private:
	virtual GamePadController* GetGamePadController(int iGamePadID) = 0;
};