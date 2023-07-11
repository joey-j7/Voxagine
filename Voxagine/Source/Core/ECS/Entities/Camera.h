#pragma once
#include "Core/ECS/Entity.h"

#include "Core/Math.h"

class Camera : public Entity
{
public:
	Camera(World* pWorld);
	virtual ~Camera();

	virtual void Awake() override;
	virtual void PreTick() override;
	virtual void PostTick(float fDeltaTime) override;

	const float& GetProjectionValue() const { return m_fProjectionValue; };
	float GetAspectRatio() const { return m_fAspectRatio; };
	void SetAspectRatio(float ratio) { m_fAspectRatio = ratio; }

	void SetMainCamera(bool bIsMainCamera) { m_bIsMainCamera = bIsMainCamera; }
	bool IsMainCamera() const { return m_bIsMainCamera; }

	void SetCameraOffset(Vector3 offset) { m_cameraOffset = offset; }
	Vector3 GetCameraOffset() const { return m_cameraOffset; }

	const Matrix4& GetMVP() const { return m_MVP; };
	const Matrix4& GetMV() const { return m_ModelView; };
	const Matrix4& GetView() const { return m_ViewMatrix; };
	const Matrix4& GetProjection() const { return m_Projection; };

	bool IsUpdated() const;

	void ForceUpdate();
	void Recalculate();

	void SetRecalculated(bool bIsUpdated);

	bool IsOrthographic() const { return m_bIsOrthographic; }
	void SetOrthographic(bool bIsOrthographic);

	float GetNearPlane() const { return m_fNearPlane; }
	float GetFarPlane() const { return m_fFarPlane; }

	// Converts screen coordinates to a world direction
	Vector3 ScreenToWorld(const Vector2& screenCoord);

	// Converts world to screen coordinates
	Vector2 WorldToScreenPoint(const Vector3& worldPos);

private:
	void CalculateProjection(uint32_t uiWindowSizeX, uint32_t uiWindowSizeY, IVector2 resizeDelta);

	float m_fProjectionValue;
	Vector3 m_cameraOffset;

	Matrix4 m_Projection;
	Matrix4 m_ModelView;
	Matrix4 m_ViewMatrix;
	Matrix4 m_MVP;

	/* Checks if one of the camera's matrices have been updated this frame */
	bool m_bIsRecalculated = false;
	bool m_bIsMainCamera = false;
	bool m_bIsOrthographic = false;

	float m_fFieldofView = 60.f;

	float m_fNearPlane = 0.1f;
	float m_fFarPlane = 10000.0f;

	float m_fAspectRatio = 16.f / 9.f;

protected:
	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND
};