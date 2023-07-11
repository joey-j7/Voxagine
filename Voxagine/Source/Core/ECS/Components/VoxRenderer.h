#pragma once

#include "Core/ECS/Component.h"

#include "Core/Math.h"

#include <External/rttr/type>

#include "Core/ECS/Systems/Physics/VoxelGrid.h"
#include "Core/ECS/Systems/Physics/Box.h"

class RenderSystem;

enum RenderLayer
{
	RL_DEFAULT,
	RL_ENTITY,
	RL_STATIC_ENTITY,
	RL_PARTICLES
};

enum RenderState {
	RS_DEFAULT,			// Shows voxels normally
	RS_GRID_LINES,		// Adds grid outline to voxels
	RS_SELECTION_LINES,	// Adds grid selection outline to voxels
};

class VoxModel;
class VoxAnimator;
struct VoxFrame;

class VoxRenderer : public Component
{
public:
	friend class RenderSystem;
	friend class VoxAnimator;
	friend class VoxelBaker;
	friend class VoxFrameEmitter;
	friend class EditorRenderMapper;

	struct BakeData {
		uint32_t* Positions = nullptr;
		uint32_t Size = 0;

		Vector3 WorldOffset = Vector3(0.f, 0.f, 0.f);
		Vector3 LastLocation = Vector3(0.f, 0.f, 0.f);
		Vector3 LastScale = Vector3(1.f, 1.f, 1.f);
		Quaternion LastRotation;

		bool IsEnabled = true;
		bool IsStatic = false;
		bool Updated = false;
	};

	Event<VoxRenderer*> FrameChanged;

	VoxRenderer(Entity* pOwner);
	~VoxRenderer();

	RenderLayer GetLayer() const { return m_RenderLayer; }
	RenderState GetState() const { return m_RenderState; }
	const VoxFrame* GetFrame() const { return m_pFrame; }

	std::string GetModelFilePath() const;

	void SetModelFilePath(std::string filePath);
	void SetFrame(const VoxFrame* pModel, bool bIncrementRef = true);

	void SetModel(const VoxModel* pModel, bool bIncrementRef = false);

	VColor GetOverrideColor() const { return m_OverrideColor; };
	void SetOverrideColor(VColor overrideColor);

	void SetLayer(RenderLayer layer) { m_RenderLayer = layer; }
	void SetState(RenderState state) { m_RenderState = state; }

	/* Determines whether the voxel positions should be rounded when rotated diagonally */
	bool IsAxisRounded() const { return m_bAxisRounded; }
	void SetAxisRounded(bool bAxisRounded) { m_bAxisRounded = bAxisRounded; }

	/* Determines whether the rotation angle should be limited to 90 degrees altogether */
	bool IsRotationAngleLimited() const { return m_uiRotationLimit != 0; }

	uint32_t GetRotationAngleLimit() const { return m_uiRotationLimit; }
	void SetRotationAngleLimit(uint32_t uiRotationAngleLimit) { m_uiRotationLimit = uiRotationAngleLimit % 360; }

	bool IsFrameChanged() const { return m_bIsFrameChanged; };
	void ResetFrameChanged() { m_bIsFrameChanged = false; };

	bool UpdateRequested() const { return m_bUpdateRequested; }
	void RequestUpdate() { m_bUpdateRequested = true; }

	bool DrawBoundsEnabled() const { return m_bDrawBounds; };
	Box GetBounds() const;

	bool IsChunkInstanceLoaded() const { return m_bIsChunkInstanceLoaded; }
	void SetChunkInstanceLoaded(bool bChunkLoaded) { m_bIsChunkInstanceLoaded = bChunkLoaded; }

private:
	void ResetModel();

private:
	const VoxFrame* m_pFrame = nullptr;
	BakeData m_BakeData;

	VColor m_OverrideColor = VColor(0);

	RenderLayer m_RenderLayer = RL_DEFAULT;
	RenderState m_RenderState = RS_SELECTION_LINES;

	std::string m_modelFilePath = "";

	uint32_t m_uiRotationLimit = 90;

	bool m_bAxisRounded = false;
	bool m_bDrawBounds = true;

	bool m_bUpdateRequested = false;
	bool m_bIsFrameChanged = false;
	bool m_bIsChunkInstanceLoaded = false;

	RTTR_ENABLE(Component)
};