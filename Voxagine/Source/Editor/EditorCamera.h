#pragma once

#include "Core/ECS/Entities/Camera.h"

class Editor;
class InputHandler;
class InputContextNew;

class EditorCamera : public Camera
{
public:
	EditorCamera(World* pWorld, Editor* pEditor);
	virtual ~EditorCamera();

	virtual void Awake() override;
	virtual void PreTick() override;
	virtual void PostTick(float fDelta) override;
	virtual void PostFixedTick(const GameTimer& timer) override;

	void LockThisFrame(bool bLock);

	Editor* GetEditor();

private:
	void ApplyPositionalCorrection();

private:
	Editor* m_pEditor = nullptr;
	InputContextNew* m_pInputContext = nullptr;
	uint64_t m_uilBindIDMouseAction;

	bool m_bLeftClickAction = false;
	bool m_bLeftClickActionValid = false;

	bool m_bRightClickAction = false;
	bool m_bRightClickActionValid = false;

	float m_fRotationSpeed = 5.f;
	float m_fMaxRotationSpeed = 1000.f;

	float m_fTranslationSpeed = 200.f;
	float m_fMaxTranslationSpeed = 3000.f;

	float m_fScrollSpeed = 800.f;

	float m_fMaxWorldDistance = 250.f;

	Vector2 m_AccumulateMouseDelta = Vector2(0.f);
	float m_fAccumulateScrollDelta = 0.f;

	bool m_bFrameLock = false;
};