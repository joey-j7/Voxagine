#include "pch.h"
#include "EditorCamera.h"

#include "Editor/Editor.h"
#include "Core/Application.h"
#include "Core/ECS/Systems/Physics/HitResult.h"

#include "Core/Platform/Input/Temp/InputContextNew.h"
#include "Core/ECS/World.h"
#include <Core/Platform/Platform.h>
#include "Core/ECS/Entity.h"
#include "Core/Settings.h"

#include "Editor/EditorWorld.h"
#include "Core/ECS/Systems/Physics/VoxelGrid.h"
#include "Core/ECS/Systems/Chunk/ChunkSystem.h"

EditorCamera::EditorCamera(World * pWorld, Editor* pEditor)
	: Camera(pWorld)
	, m_pEditor(pEditor)
{
}

EditorCamera::~EditorCamera()
{
	GetWorld()->GetApplication()->GetPlatform().GetInputContext()->UnBindAction(m_uilBindIDMouseAction);
}

void EditorCamera::Awake()
{
	Camera::Awake();

	SetName("Editor Camera");

	m_pInputContext = GetWorld()->GetApplication()->GetPlatform().GetInputContext();

	UVector2 chunkWorldSize = m_pEditor->GetEditorWorld()->GetChunkSystem()->GetWorldSize();
	UVector3 worldSize;
	GetWorld()->GetVoxelGrid()->GetDimensions(worldSize.x, worldSize.y, worldSize.z);

	m_fMaxWorldDistance = static_cast<float>(sqrt(chunkWorldSize.x * chunkWorldSize.x + worldSize.y * worldSize.y + chunkWorldSize.y * chunkWorldSize.y));
	std::vector<uint64_t> m_ActionBindings;

	m_pInputContext->BindAction(EDITOR_INPUT_LAYER_NAME, "Mouse_Action", IKS_RELEASED, m_ActionBindings, BMT_GLOBAL, [this]() {
		if (IsEnabled())
		{
			if (!GetEditor()->IsMouseHoveringEditorWindows() && !GetEditor()->IsModifyingSelectedEntityTransform())
			{
				IVector2 MousePosition = GetWorld()->GetApplication()->GetPlatform().GetInputContext()->GetMousePosition();
				Vector3 MouseDir = ScreenToWorld(Vector2(static_cast<float>(MousePosition.x), static_cast<float>(MousePosition.y)));

				HitResult Result;
				if (GetWorld()->RayCast(GetTransform()->GetPosition(), MouseDir, Result, 400.f))
					GetEditor()->SetSelectedEntity(Result.HitEntity);
			}
		}
	});

	m_uilBindIDMouseAction = m_ActionBindings[0];

	ApplyPositionalCorrection();
	Recalculate();
}

void EditorCamera::PreTick()
{
	Camera::PreTick();
}

void EditorCamera::PostTick(float fDelta)
{
	Camera::PostTick(fDelta);
	
	m_AccumulateMouseDelta += m_pInputContext->GetMousePositionDelta();
	m_fAccumulateScrollDelta += m_pInputContext->GetMouseWheelDelta();
	
}

void EditorCamera::PostFixedTick(const GameTimer& timer)
{
	Entity::PostFixedTick(timer);

	GetWorld()->GetChunkSystem()->SetCameraLoadOffset(Vector3(0.f));

	if (m_bFrameLock || GetEditor()->IsModifyingSelectedEntityTransform())
	{
		m_bFrameLock = false;
		Recalculate();
		m_AccumulateMouseDelta = Vector2(0.f);
		m_fAccumulateScrollDelta = 0.f;
		return;
	}

	const bool bRightClicked = m_pInputContext->IsMouseButtonDownRight();

	if (!m_bRightClickAction && bRightClicked)
	{
		m_bRightClickActionValid = (!GetEditor()->IsMouseHoveringEditorWindows()) ? true : false;
		m_bRightClickAction = true;
	}
	else if (!bRightClicked)
	{
		m_bRightClickAction = false;
		m_bRightClickActionValid = false;
	}

	if (m_pInputContext->GetActiveBindingMap()->Name == EDITOR_INPUT_LAYER_NAME)
	{
		float deltaTime = static_cast<float>(timer.GetElapsedSeconds());
		float speedMultiplier = 1.0f;

		if (m_pInputContext->IsKeyHeld(IK_LEFTSHIFT))
		{
			speedMultiplier = 2.0f;
		}

		/* Get Input Axis values*/
		Vector3 CamerTranslation = Vector3(0.f);

		CamerTranslation.x = m_pInputContext->GetAxisValue("EditorCamera_Left", BMT_GLOBAL).Value + m_pInputContext->GetAxisValue("EditorCamera_Right", BMT_GLOBAL).Value;
		CamerTranslation.z = m_pInputContext->GetAxisValue("EditorCamera_Forward", BMT_GLOBAL).Value + m_pInputContext->GetAxisValue("EditorCamera_Backward", BMT_GLOBAL).Value;

		/* Get input */
		const float fMouseWheelDelta = m_fAccumulateScrollDelta;
		m_fAccumulateScrollDelta = 0.f;

		Vector2 mouseDelta = m_AccumulateMouseDelta;
		m_AccumulateMouseDelta = Vector2(0.f);

		const bool bShouldUpdate = fMouseWheelDelta != 0 || ((bRightClicked) && (glm::length(mouseDelta) != 0 || glm::length(CamerTranslation) != 0));

		if (!bShouldUpdate) {
			Recalculate();
			return;
		}

		/* Translate position on mouse scroll */
		if (fMouseWheelDelta != 0 && !GetEditor()->IsMouseHoveringEditorWindows() && !GetEditor()->IsModifyingSelectedEntityTransform())
			GetTransform()->Translate(GetTransform()->GetForward() * fMouseWheelDelta / abs(fMouseWheelDelta) * deltaTime * m_fScrollSpeed);

		/* Correct rotational speed */
		Vector2 rotation = mouseDelta;
		rotation *= m_fRotationSpeed;

		rotation = glm::clamp(rotation, -Vector2(1.f) * m_fMaxRotationSpeed, Vector2(1.f) * m_fMaxRotationSpeed);
		rotation *= deltaTime;

		if (glm::length(CamerTranslation) != 0 && bRightClicked && m_bRightClickActionValid)
		{
			CamerTranslation = glm::normalize(CamerTranslation);
			Vector3 RotationForward = glm::rotate(GetTransform()->GetRotation(), CamerTranslation);
			//Vector3::Transform(CamerTranslation, GetTransform()->GetRotation());

			GetTransform()->Translate(RotationForward * m_fTranslationSpeed * deltaTime);
		}

		if (bRightClicked && m_bRightClickActionValid)
		{
			GetTransform()->Rotate(Vector3(0, rotation.x, 0));
			GetTransform()->LocalRotate(Vector3(rotation.y, 0, 0));
		}

		ApplyPositionalCorrection();
		Recalculate();
	}
}

void EditorCamera::LockThisFrame(bool bLock)
{
	m_bFrameLock = true;
}

Editor * EditorCamera::GetEditor()
{
	return m_pEditor;
}

void EditorCamera::ApplyPositionalCorrection()
{
	/* Clamp values */
	Vector3 position = GetTransform()->GetPosition();

	UVector3 worldSize;
	UVector2 chunkWorldSize = m_pEditor->GetEditorWorld()->GetChunkSystem()->GetWorldSize();
	m_pEditor->GetEditorWorld()->GetVoxelGrid()->GetDimensions(worldSize.x, worldSize.y, worldSize.z);
	worldSize.x = chunkWorldSize.x;
	worldSize.y = chunkWorldSize.y;

	Vector3 worldCenter = Vector3(
		worldSize.x * 0.5f,
		worldSize.y * 0.5f,
		worldSize.z * 0.5f
	);

	float distance = glm::distance(
		position,
		worldCenter
	);

	/* Make sure the camera doesn't get too far from the world */
	if (distance > m_fMaxWorldDistance)
	{
		float delta = distance - m_fMaxWorldDistance;

		Vector3 direction = glm::normalize(worldCenter - position);
		GetTransform()->Translate(direction * delta);
	}

	/* Make sure the camera position doesn't get below the world */
	position = GetTransform()->GetPosition();

	if (position.y < 10.0f) {
		position.y = 10.0f;

		GetTransform()->SetPosition(position);
	}
}
