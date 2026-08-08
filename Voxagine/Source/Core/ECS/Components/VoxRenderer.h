#pragma once

#include "Core/ECS/Component.h"

#include "Core/Math.h"

#include <rttr/type>

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

		/* RenderContext::GetVoxelGeneration at the moment Positions was
		   written. Zero means "never baked", which never matches. */
		uint32_t Generation = 0;

		/* Everything the stamped voxels are a function of.
		   ForEachStampedVoxel reads exactly two things: the VoxelStampTransform
		   below, and three values off the renderer - its frame, its override
		   colour and its render state. So two stamps with equal keys produce
		   an identical sequence of (voxel, colour) pairs, and re-baking one
		   over the other writes what is already there.

		   Deliberately the stamp's own inputs rather than the transform's:
		   position, rotation and scale reach the stamp through quantization
		   and a floor, so a renderer can move without moving a single voxel,
		   which is the common case at a world load. */
		struct StampKey
		{
			Vector3 Origin = Vector3(0.f);
			Quaternion Rotation;
			Vector3 Scale = Vector3(1.f);
			Vector3 RoundedScale = Vector3(0.f);

			const VoxFrame* Frame = nullptr;
			uint32_t OverrideColor = 0;
			int32_t State = -1;

			bool operator==(const StampKey& other) const
			{
				return Frame == other.Frame &&
					OverrideColor == other.OverrideColor &&
					State == other.State &&
					Origin == other.Origin &&
					Scale == other.Scale &&
					RoundedScale == other.RoundedScale &&
					Rotation.x == other.Rotation.x &&
					Rotation.y == other.Rotation.y &&
					Rotation.z == other.Rotation.z &&
					Rotation.w == other.Rotation.w;
			}
		};

		/* Default RoundedScale of zero is a stamp no real one can equal, so a
		   BakeData that has never been through Occupy never matches. */
		StampKey Stamp;

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