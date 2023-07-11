#pragma once
#include "Core/ECS/Component.h"

#include "Core/Math.h"

#include <External/rttr/type>
#include <External/rttr/registration_friend> 

static const float PI = 3.14159265359f;

static const float DEG2RAD = 0.0174532925f;
static const float RAD2DEG = 57.2957795f;

class Transform : public Component
{
public:
	Transform(Entity* pOwner);

	void UpdateMatrix();

	const Matrix4& GetMatrix() const;
	const Matrix4& GetLocalMatrix() const;

	void SetFromMatrix(const Matrix4& matrix, bool bLocal = false);
	void SetFromLocalMatrix(const Matrix4& matrix) { SetFromMatrix(matrix, true); };

	Vector3 GetPosition() const;
	Vector3 GetLocalPosition() const;

	Vector3 GetScale() const;
	Vector3 GetLocalScale() const;

	Vector3 GetEulerAngles() const;
	Vector3 GetLocalEulerAngles() const;

	Quaternion GetRotation() const;
	Quaternion GetLocalRotation() const;

	bool IsDirty() { return m_bIsDirty; }
	bool IsUpdated() const { return m_bIsUpdated; };

	const Vector3& GetForward() const;
	const Vector3& GetRight() const;
	const Vector3& GetUp() const;

	void Translate(Vector3 translation, bool bLocal = false);

	inline void GlobalTranslate(Vector3 translation) { Translate(translation, false); };
	inline void LocalTranslate(Vector3 translation) { Translate(translation, true); };

	void Rotate(Vector3 eulerAngles, bool bLocal = false);

	inline void GlobalRotate(Vector3 eulerAngles) { Rotate(eulerAngles, false); };
	inline void LocalRotate(Vector3 eulerAngles) { Rotate(eulerAngles, true); };

	void SetPosition(Vector3 pos, bool bLocal = false);

	inline void SetGlobalPosition(Vector3 pos) { SetPosition(pos, false); };
	inline void SetLocalPosition(Vector3 pos) { SetPosition(pos, true); };

	void SetScale(Vector3 scale, bool bLocal = false);\

	inline void SetGlobalScale(Vector3 scale) { SetScale(scale, false); };
	inline void SetLocalScale(Vector3 scale) { SetScale(scale, true); };

	void SetRotation(Quaternion quat, bool bLocal = false);

	inline void SetGlobalRotation(Quaternion quat) { SetRotation(quat, false); };
	inline void SetLocalRotation(Quaternion quat) { SetRotation(quat, true); };

	void SetEulerAngles(Vector3 eulerAngles, bool bLocal = false);

	inline void SetGlobalEulerAngles(Vector3 eulerAngles) { SetEulerAngles(eulerAngles, false); };
	inline void SetLocalEulerAngles(Vector3 eulerAngles) { SetEulerAngles(eulerAngles, true); };

	void SetDirty(bool bDirty);
	void SetUpdated(bool bUpdated);

private:
	Quaternion m_Rotation = Quaternion();
	Vector3 m_EulerAngles = Vector3(0.f);
	Vector3 m_Position = Vector3(0.f);
	Vector3 m_Scale = Vector3(1.f);

	Quaternion m_LocalRotation = Quaternion();
	Vector3 m_LocalEulerAngles = Vector3(0.f);
	Vector3 m_LocalPosition = Vector3(0.f);
	Vector3 m_LocalScale = Vector3(1.f);

	Vector3 m_Forward = Vector3(0.f, 0.f, 1.f);
	Vector3 m_Right = Vector3(1.f, 0.f, 0.f);
	Vector3 m_Up = Vector3(0.f, 1.f, 0.f);

	Matrix4 m_TransformationMatrix = Matrix4(1.f);
	Matrix4 m_LocalTransformationMatrix = Matrix4(1.f);

	bool m_bIsDirty = true;
	bool m_bIsUpdated = true;

	RTTR_ENABLE(Component)
	RTTR_REGISTRATION_FRIEND
};