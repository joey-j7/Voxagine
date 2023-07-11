#pragma once

#include "Core\ECS\Entities\UI\Canvas.h"

#include "Core/Math.h"

#include "Core/Objects/VClass.h"

#include <External/rttr/type>

class World;
class SpriteRenderer;

struct LevelSelectWorld : VClass {
	std::string sWorldFilePath;
	std::string sWorldImageFilePath;

	bool bUnlocked = false;
	bool bNewUnlocked = false;

	// RTTR for VClass parent must be enabled so that the dropdown option for the struct appears
	RTTR_ENABLE(VClass);
	RTTR_REGISTRATION_FRIEND;
};

class LevelSelectCanvas :
	public Canvas
{

public:
	LevelSelectCanvas(World*);
	virtual ~LevelSelectCanvas();

	void SetNavigatable(bool) override { Canvas::SetNavigatable(true); };

	void Start() override;

	void Tick(float) override;

protected:

	virtual void SetFocusLeft() override;
	virtual void SetFocusRight() override;

	virtual void OnPressed() override {};
	virtual void OnPressedRepeat() override {};
	virtual void OnReleased() override;

private:

	void UpdateSelectedWorld();
	LevelSelectWorld& GetCurrentLevelSelectWorld() { return m_LevelSelectWorlds[m_iCurrentlySelectedWorld]; }

	void SetLevelSelectWorlds(std::vector<LevelSelectWorld> levelSelectWorlds);
	std::vector<LevelSelectWorld> GetLevelSelectWorlds() const { return m_LevelSelectWorlds; };

	void UpdateTransitionOffset(float);

private:

	bool m_bCheckForWorldUnlocked = true;

	SpriteRenderer* m_pSpriteRenderer = nullptr;

	SpriteRenderer* m_pLeftUnlockWorldSpriteRenderer = nullptr;
	SpriteRenderer* m_pRightUnlockWorldSpriteRenderer = nullptr;

	Vector2 m_MovementDirection = Vector2(1.f, 0.f);
	float m_MovementOffset = 0.f; 
	float m_fMovementTransitionDuration = 3.f;

	bool m_bOpening = false;
	float m_fCurrentMovementTransitionProgress = 1.f;

	std::vector<LevelSelectWorld> m_LevelSelectWorlds;
	int m_iCurrentlySelectedWorld = 0;

	bool m_bLoopWorlds = true;

	bool m_bGotoMain = false;
	std::string m_sNextWorld = "";

	RTTR_ENABLE(Canvas);
	RTTR_REGISTRATION_FRIEND
};

