#pragma once
#include "Core/ECS/Entity.h"
#include "Core/Math.h"

class AudioSource;

class CameraMultiplayer : public Entity
{
public:
	Vector3 m_targetRotation;
	float m_fMinTargetDistance;
	float m_fMaxTargetDistance;
	Entity* m_pPlayer1;
	Entity* m_pPlayer2;
	float m_fMovementSmoothing;
	float m_fZoomOutBound;
	float m_fZoomInBound;
	float m_fZoomSpeed;
	Vector2 m_fPlayerMovementBound;
	Vector2 m_fPlayerBoundOffset;
	Vector3 m_fMaxShakeAngle;
	float m_fShakeFalloff;
	Vector3 m_ChunkLoadOffset;

private:
	Camera* m_pMainCamera;
	bool m_updateCamera;
	Vector3 m_targetPosition;
	float m_fTargetDistance;
	Vector3 m_player1PrevPosition;
	Vector3 m_player2PrevPosition;
	float m_fShakeStrength;

	AudioSource* m_pAudioSource = nullptr;

public:
	CameraMultiplayer(World* pWorld);
	virtual ~CameraMultiplayer() = default;

	void SetPlayer1(Entity* pEntity);
	Entity* GetPlayer1() { return m_pPlayer1; }

	void SetPlayer2(Entity* pEntity);
	Entity* GetPlayer2() { return m_pPlayer2; }

	void Awake() override;
	void Start() override;
	void PreTick() override;
	void PostFixedTick(const GameTimer& timer) override;

	void AddCameraShake(float strength);
	void SetCameraShake(float strength);
	float GetCameraShake() const;

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND

private:
	void calculatePlayersScreenPos(Vector2& screenPosPlayer1, Vector2& screenPosPlayer2) const;
	Vector3 calculateLookAtPos() const;
	float calculateTargetDistance(const Vector2& screenPosPlayer1, const Vector2& screenPosPlayer2) const;
	void lockPlayersInView(const Vector2& screenPosPlayer1, const Vector2& screenPosPlayer2);
	void calculateChunkLoadOffset(Vector3& loadOffset);
};