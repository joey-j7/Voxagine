#pragma once
#include "Core/ECS/Components/BehaviorScript.h"
#include "Core/ECS/Systems/Physics/VoxelGrid.h"

class VoxRenderer;
class FlashBehavior : public BehaviorScript
{
	RTTR_ENABLE(BehaviorScript);
	RTTR_REGISTRATION_FRIEND;
public:
	FlashBehavior(Entity* pOwner) : BehaviorScript(pOwner) {}
	virtual ~FlashBehavior() { m_pRenderer = nullptr; /* we are not the owner of the renderer*/ };

	void Awake() override;
	void StartFlashing();

	double Flash();

	VColor FlashingColor = VColor(static_cast<unsigned char>(255), 255u, 255u, 255u);

	int iTimesToFlash = 3;
	double dFlashDelay = 0.025;

private:
	VColor m_CurrentColor = VColor();

	VoxRenderer* m_pRenderer = nullptr;

	bool m_bFlashing = false;
	bool m_bStarted = false;
	int m_iCurrentCount = 0;
};
