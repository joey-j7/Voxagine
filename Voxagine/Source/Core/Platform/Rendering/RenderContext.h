#pragma once

#include "Core/ECS/Systems/Rendering/Buffers/RenderData.h"

#include "Core/Event.h"

#include "Core/ECS/Systems/Rendering/Buffers/RenderBuffer.h"
#include "Core/ECS/Systems/Rendering/Buffers/Structures/StructuredVoxelBuffer.h"

#include "Core/Platform/Rendering/RenderDefines.h"
#include "Core/Platform/Rendering/Managers/ModelManagerInc.h"
#include "Core/Platform/Rendering/Managers/TextureManagerInc.h"
#include "Core/Platform/Rendering/CommandEngineInc.h"
#include "Core/Platform/Rendering/RenderPassInc.h"
#include "Core/Platform/Rendering/ComputePassInc.h"

#include "Core/Math.h"
#include "Core/VColors.h"

#include <stdint.h>
#include <string>
#include <memory>
#include <unordered_map>

#include "Core/Platform/Rendering/Objects/Buffer.h"
#include "Core/Platform/Rendering/Objects/Sampler.h"
#include "Core/Platform/Rendering/Objects/Shader.h"
#include "Core/Platform/Rendering/Objects/View.h"
#include "Core/Platform/Rendering/Objects/Mapper.h"

#include "Core/Resources/Formats/TextureReference.h"
#include "Core/Resources/Formats/ShaderReference.h"

#include "Core/Platform/Rendering/RenderAlignment.h"
#include "Core/Platform/Rendering/VoxelBrickGrid.h"
#include "Core/Platform/Rendering/FarFieldVolume.h"

class Platform;
class WindowContext;
class RenderPass;

class ParticlePass;

class Settings;
class Camera;

class Particle;
class Mapper;

class VoxRenderer;

struct CameraRenderData {
	CameraRenderData() = default;
	CameraRenderData(
		const Matrix4& mvp,
		const Matrix4& modelView,
		const Matrix4& view,
		const Matrix4& projection,

		float fProjectionValue,
		float fAspectRatio,
		bool bIsOrthographic,
		bool bIsUpdated,

		Vector4 worldPos,
		Vector4 cameraOffset
	) {
		/* Used for debug and depth rendering */
		m_MVP = mvp;

		m_ModelView = modelView;
		m_View = view;
		m_Projection = projection;

		m_fProjectionValue = fProjectionValue;
		m_fAspectRatio = fAspectRatio;
		m_bIsOrthographic = bIsOrthographic;
		m_bIsUpdated = bIsUpdated;

		m_WorldPos = worldPos;
		m_CameraOffset = cameraOffset;
	}

	/* Used for debug and depth rendering */
	Matrix4 m_MVP;

	/* Used for voxel rendering */
	Matrix4 m_ModelView;

	Matrix4 m_View;
	Matrix4 m_Projection;

	float m_fProjectionValue;
	float m_fAspectRatio;
	bool m_bIsOrthographic;
	bool m_bIsUpdated;

	Vector4 m_WorldPos;
	Vector4 m_CameraOffset;
};

struct DebugLine {
	Vector3 m_Start;
	Vector3 m_End;
	VColor m_Color = VColors::Green;
};

struct DebugSphere {
	Vector3 m_Center;
	float m_fRadius;
	VColor m_Color = VColors::Green;
};

struct DebugBox {
	Vector3 m_Center;
	Vector3 m_Extents;
	VColor m_Color = VColors::Green;
};

struct SpriteData {
	Matrix4 Model;
	uint32_t TextureID;

	Vector4 Color;

	Vector2 Offset;
	Vector2 Size;

	uint32_t Alignment;
	uint32_t ScreenAlignment;

	uint32_t IsScreen;

	int32_t Layer;

	Vector2 TextureRepeat;

	Vector2 cullStart;
	Vector2 cullEnd;

	uint32_t padding;
};

struct ParticleMapperData {
	ParticleMapperData(Mapper* pMapper, uint32_t uiCount) : m_pMapper(pMapper), m_uiCount(uiCount) {}
	Mapper* m_pMapper = nullptr;
	uint32_t m_uiCount;
};

struct TextureReadData
{
	~TextureReadData() { delete[] m_Data; }

	uint32_t* m_Data = nullptr;
	UVector2 m_Dimensions = UVector2(0, 0);
};

class RenderContext
{
public:
	friend class TextureReference;
	friend class ShaderReference;
	friend class PhysicsSystem;
	friend class ParticlePass;
	friend class DebugPass;
	friend class RenderSystem;
	friend class Editor;

	// Maximum queued frames on the GPU
	static const uint32_t m_uiFrameCount = 2;

	virtual ~RenderContext();

	static void Report();

	virtual void Initialize();
	virtual void Deinitialize() {};

	PRenderContext* Get();

	virtual TextureReadData* ReadTexture(const std::string& texturePath);
	virtual void DestroyShader(const ShaderReference* pTextureReference) = 0;

	virtual void WaitForGPU();

	/* Submit data to the draw list */
	virtual void Submit(const RenderData& renderData);

	virtual void Submit(const DebugLine& renderData);
	virtual void Submit(const DebugSphere& renderData);
	virtual void Submit(const DebugBox& renderData);

	virtual void Submit(const SpriteData& renderData);

	virtual void Submit(StructuredVoxelBuffer& renderData);

	void SortAABBs();

	void EnableDebugLines(bool bEnabled);

	/* Window size reduced to the locked aspect ratio from Settings; equal to
	   the input when nothing is locked. */
	UVector2 ConstrainToAspectRatio(uint32_t uiWidth, uint32_t uiHeight) const;

	/* Window pixel to 0..1 across the presented image. A locked aspect ratio
	   centres a smaller render target in the window, so the two spaces differ
	   by the black bar; identity when nothing is locked. */
	Vector2 WindowToRenderNormalized(const Vector2& v2WindowPoint) const;

	bool ResizeWorldBuffer();
	inline bool ModifyVoxel(uint32_t uiID, uint32_t uiColor, bool bOverwrite = true)
	{
		/* The callers derive this ID from float world positions, so a bad
		   transform reaches here as an index rather than as a crash at the
		   source. Writing outside the mapped voxel buffer corrupts whatever
		   the allocator put next to it. */
		if (uiID >= GetVoxelDataSize())
			return false;

		/* The old *occupancy* comes from the brick grid's CPU-side bitmap, not
		   from the mapping. The mapping prefers ReBAR, so reading a voxel back
		   is a PCIe read of VRAM, and the bake path performs millions of them:
		   this read alone was 5.3 seconds of a world load and 74 ms of a chunk
		   load, measured. See VoxelBrickGrid.

		   That drops the old redundant-write guard (uiOldColor != uiColor),
		   which needed the colour rather than the occupancy. Writing the same
		   value twice is a streaming store into write-combined memory and
		   costs less than learning it was unnecessary would. */
		const bool bWasOccupied = m_BrickGrid.IsOccupied(uiID);

		/* "Do not overwrite" means do not overwrite anything solid. The old
		   form tested the whole word against zero; occupancy is alpha > 0
		   (rule 3), and nothing writes a colour with a zero alpha byte, so the
		   two agree on every value the engine produces. */
		if (!bOverwrite && bWasOccupied)
			return false;

		m_pVoxelData[uiID] = uiColor;
		m_BrickGrid.SetVoxel(uiID, (uiColor >> 24) != 0);

		m_bWorldUpdated = true;

		return true;
	}

	inline void ModifyVoxelFast(uint32_t uiID, uint32_t uiColor)
	{
		if (uiID >= GetVoxelDataSize())
			return;

		/* Write-only with respect to the mapping, as the name promises: the
		   old occupancy the brick count needs comes from the bitmap, in
		   ordinary cached memory. See ModifyVoxel above. */
		m_pVoxelData[uiID] = uiColor;
		m_BrickGrid.SetVoxel(uiID, (uiColor >> 24) != 0);

		m_bWorldUpdated = true;
	}

	void UpdateWorld() { m_bWorldUpdated = true; }
	uint32_t GetVoxelDataSize() { return m_pVoxelMapper->GetInfo().m_uiElementCount; }

	/* Bumped every time the voxel buffer stops holding what was stamped into
	   it - a clear or a resize. A baker records it alongside the positions it
	   wrote, and comparing it is how VoxelBaker::Bake tells "the world was
	   reset under me, re-stamp" apart from "something asked for an update but
	   my voxels are still there". Without it a forced update has to assume the
	   worst and re-stamp every renderer, which at a world load is a clear and
	   an occupy that exactly cancel. */
	uint32_t GetVoxelGeneration() const { return m_uiVoxelGeneration; }

	uint32_t GetVoxel(uint32_t uiID) const;
	uint32_t* GetVoxelData() { return m_pVoxelMapper->GetData(); }
	uint32_t* GetVoxelBackData() { return m_pVoxelMapper->GetBackBufferData(); }
	const uint32_t* GetVoxelData() const { return m_pVoxelMapper->GetData(); }
	Mapper* GetVoxelMapper() const { return m_pVoxelMapper; }
	void ClearVoxels();

	/* Coarse occupancy over the same window, for the marcher's outer walk.
	   Kept current by ModifyVoxel/ModifyVoxelFast above and, in bulk, by
	   ChunkSystem::RenderChunk. See VoxelBrickGrid. */
	VoxelBrickGrid& GetBrickGrid() { return m_BrickGrid; }
	Mapper* GetBrickMapper() const { return m_pBrickMapper; }

	/* Recomputes the whole brick grid from the voxel buffer and logs anything
	   that disagrees. On demand only - it reads the entire window back out of
	   uncached memory. */
	uint32_t ValidateBrickGrid();

	/* Cross-checks the far field's placement against the resident window, which
	   holds the same geometry at full resolution. See FarFieldVolume::Validate.
	   On demand only, for the same reason. */
	uint32_t ValidateFarField();

	/* Far-field LOD volume (RENDERING_PLAN.md phase 4): the whole level at a
	   quarter resolution, so a ray that leaves the 3x3 detail window has
	   something to hit. See FarFieldVolume. */
	FarFieldVolume& GetFarField() { return m_FarField; }

	/* Rebuilds the volume for pWorld's level and pushes it to the GPU. Sizes
	   the mappers, so it must run before anything samples them. */
	void BuildFarField(class World* pWorld);

	/* The cell grid the shader marches, or (0,0,0) when there is no far field -
	   which is what a level whose window already covers it reports, and what
	   the toggle below reports when off. */
	UVector3 GetFarFieldShaderGridSize() const;

	/* Runtime toggle: what the far field costs depends entirely on how much
	   sky is on screen, so an A/B is only meaningful without moving the camera
	   between measurements. */
	bool IsFarFieldEnabled() const { return m_bFarFieldEnabled; }
	void SetFarFieldEnabled(bool bEnabled) { m_bFarFieldEnabled = bEnabled; ForceCameraDataUpdate(); }

	/* Present pacing, serialized as Settings::EnableVSync.
	 *
	 * On when the displayed frames should each carry an equal slice of world
	 * time, off for the lowest latency. Neither is free: mailbox at 200 fps on
	 * a 60 Hz display advances the world 15 ms, then 20, then 15 between shown
	 * frames, which reads as skipping even though every frame is on time.
	 * Runtime-toggleable for the same reason the far field is - which of the
	 * two is worse is a judgement, and judging it means switching without
	 * anything else changing. */
	virtual bool IsVSyncEnabled() const { return false; }
	virtual void SetVSyncEnabled(bool bEnabled) { (void)bEnabled; }

	/* Clear the screen */
	virtual void Clear();
	virtual void FixedClear();

	/* Present all the gathered data to the screen */
	virtual bool Present();

	Platform* GetPlatform() { return m_pPlatform; }
	UVector2 GetRenderResolution() const { return UVector2(m_v2RenderResolution.x * m_fRenderScale, m_v2RenderResolution.y * m_fRenderScale); }
	UVector2 GetScreenResolution() const { return m_v2ScreenResolution; }

	float GetRenderScale() const { return m_fRenderScale; }

	CameraRenderData& GetCameraData() { return m_CameraData; }
	void SetCameraData(CameraRenderData cameraData) { m_CameraData = cameraData; }

	const std::vector<StructuredVoxelBuffer>& GetAABBList() const { return m_AABBList; }

	uint32_t GetFrameIndex() const { return m_uiFrameIndex; }
	uint32_t GetMissedFrames() const { return m_uiMissedFrames; }
	uint32_t GetDrawnFrames() const { return m_uiDrawnFrames; }

	uint32_t GetFPS() const { return m_uiFPS; }

	void ResetFrameCount() { m_uiDrawnFrames = 0; }

	PCommandEngine* GetEngine(const std::string& sName) { return m_pCommandEngines[sName].get(); }

	PRenderPass* GetRenderPass(const std::string& sName)
	{
		auto found = m_pRenderPasses.find(sName);
		return found != m_pRenderPasses.end() ? found->second.get() : nullptr;
	}

	PTextureManager* GetTextureManager() const { return m_pTextureManager.get(); }
	PModelManager* GetModelManager() const { return m_pModelManager.get(); }

	void ForceUpdate() { m_bWorldUpdated = true; };
	void ForceCameraDataUpdate() { m_bCameraDataUpdated = true; };

	void SetFadeValue(float fValue);
	float GetFadeValue() const { return m_fFader; }

	/* Resizes the context, buffers and window */
	virtual bool OnResize(uint32_t uiWidth, uint32_t uiHeight);

	Event<bool> FullscreenChanged;
	Event<uint32_t, uint32_t, IVector2> SizeChanged;

protected:
	RenderContext(Platform* pPlatform);

	struct DebugDrawLine {
		Vector4 m_Position;
		Vector4 m_Color = Vector4(0.f, 1.f, 0.f, 1.f);
	};

	void InitializeRenderLoop();

	/* Textures */
	virtual void LoadTexture(TextureReference* pTextureReference);
	virtual void DestroyTexture(const TextureReference* pTextureRef);

	/* Shaders */
	virtual void LoadShader(ShaderReference* pTextureReference) = 0;

	/* Events */
	void OnFullscreenChanged(bool bFullscreen = false);

	uint32_t m_uiFrameIndex = 0;

	Platform* m_pPlatform;
	Settings* m_pSettings = nullptr;

	std::unique_ptr<PTextureManager> m_pTextureManager = nullptr;
	std::unique_ptr<PModelManager> m_pModelManager = nullptr;

	// Render Targets
	std::unordered_map<std::string, std::unique_ptr<PRenderPass>> m_pRenderPasses;

	// Compute passes
	std::unordered_map<std::string, std::unique_ptr<PComputePass>> m_pComputePasses;

	// Command Engines
	std::unordered_map<std::string, std::unique_ptr<PCommandEngine>> m_pCommandEngines;

	// Resources
	std::vector<std::unique_ptr<View>> m_pViews;
	std::vector<std::unique_ptr<Shader>> m_pShaders;
	std::vector<std::unique_ptr<Sampler>> m_pSamplers;
	std::vector<std::unique_ptr<Mapper>> m_pMappers;

	std::unordered_map<std::string, std::unique_ptr<Buffer>> m_mBuffers;
	
	ParticlePass* m_pParticlePass = nullptr;
	uint32_t m_uiParticleCount = 0;

	Mapper* m_pVoxelMapper = nullptr;
	uint32_t* m_pVoxelData = nullptr;

	Mapper* m_pBrickMapper = nullptr;
	VoxelBrickGrid m_BrickGrid;

	/* Far field. Its own brick grid, over its own cell grid rather than the
	   window's voxels - the two never interact, and keeping them separate is
	   what lets the far-field marcher be the same two-level walk. */
	Mapper* m_pFarFieldMapper = nullptr;
	Mapper* m_pFarFieldBrickMapper = nullptr;
	FarFieldVolume m_FarField;
	VoxelBrickGrid m_FarFieldBricks;

	bool m_bFaderUpdated = false;

	// Frontend resources
	std::vector<StructuredVoxelBuffer> m_AABBList;

	std::vector<RenderData> m_RenderList;
	std::vector<SpriteData> m_SpriteList;

#if defined(EDITOR) || defined(_DEBUG)
	static const int m_iSphereResolution = 30;
	static const int m_iSphereLineCount = (m_iSphereResolution + 1) * 3;

	std::vector<DebugDrawLine> m_DebugDrawLines;
	std::vector<Vector3> m_UnitDebugSphere;
#endif

	float m_fFader = 0.f;
	float m_fFadeTime = 1.f;

	float m_fFrameTimer = 0.f;

	/* Per-frame deltas of the current second, for the [fps] line's percentile.
	   Fixed size: a frame limiter of 0.005 gives 200 a second, and anything
	   past this is a second so pathological the average is the least of it. */
	static const uint32_t k_uiMaxFrameSamples = 4096;

	float m_fFrameSamples[k_uiMaxFrameSamples] = {};
	uint32_t m_uiFrameSamples = 0;

	bool m_bIsFullscreen = false;
	/* Off by default: physics colliders and the like are a development aid, not
	   something the game or a freshly opened editor should draw. The editor's
	   View menu still toggles it - Editor::m_bRenderDebugLines starts matched
	   to this.

	   m_bDebugCleared starts false so the first frame still runs the debug
	   pass's clear path once. Post processing samples that target
	   unconditionally, so a pass that never drew and never cleared leaves it
	   undefined. */
	bool m_bDebugEnabled = false;
	bool m_bDebugCleared = false;

	bool m_bFarFieldEnabled = true;

	/* The camera of the previous upload, which is the one the voxel image post
	   processing composites was rendered with - see CameraData.hlsl's
	   sceneInvMvp. Identity until the second upload; the first frame has no
	   previous image to be out of step with. */
	struct SceneCamera
	{
		Matrix4 m_InvMVP = Matrix4(1.f);
		Vector4 m_WorldPos = Vector4(0.f);
		Vector4 m_Offset = Vector4(0.f);
	};

	SceneCamera m_PreviousSceneCamera;

	CameraRenderData m_CameraData;

	/* Consecutive Present calls that found the GPU still busy. */
	uint32_t m_uiStalledFrames = 0;

	UVector2 m_v2RenderResolution = UVector2(1, 1);
	UVector2 m_v2ScreenResolution = UVector2(1, 1);

	float m_fRenderScale = 1.0f;

	uint32_t m_uiMissedFrames = 0;
	uint32_t m_uiDrawnFrames = 0;
	uint32_t m_uiFPS = 0;

	/* See GetVoxelGeneration. Starts at 1 so a BakeData that has never been
	   written (Generation 0) always reads as stale. */
	uint32_t m_uiVoxelGeneration = 1;

	bool m_bIsDrawTextureCopied = false;
	bool m_bWorldUpdated = true;
	bool m_bCameraDataUpdated = false;
};