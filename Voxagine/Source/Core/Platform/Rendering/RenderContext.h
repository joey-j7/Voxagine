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

	bool ResizeWorldBuffer();
	inline bool ModifyVoxel(uint32_t uiID, uint32_t uiColor, bool bOverwrite = true)
	{
		uint32_t& uiOldColor = m_pVoxelData[uiID];

		if ((bOverwrite || uiOldColor == 0) && uiOldColor != uiColor) {
			uiOldColor = uiColor;
			m_bWorldUpdated = true;

			return true;
		}

		return false;
	}

	inline void ModifyVoxelFast(uint32_t uiID, uint32_t uiColor)
	{
		m_pVoxelData[uiID] = uiColor;
		m_bWorldUpdated = true;
	}

	void UpdateWorld() { m_bWorldUpdated = true; }
	uint32_t GetVoxelDataSize() { return m_pVoxelMapper->GetInfo().m_uiElementCount; }

	uint32_t GetVoxel(uint32_t uiID) const;
	uint32_t* GetVoxelData() { return m_pVoxelMapper->GetData(); }
	uint32_t* GetVoxelBackData() { return m_pVoxelMapper->GetBackBufferData(); }
	const uint32_t* GetVoxelData() const { return m_pVoxelMapper->GetData(); }
	Mapper* GetVoxelMapper() const { return m_pVoxelMapper; }
	void ClearVoxels();

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

	bool m_bIsFullscreen = false;
	bool m_bDebugEnabled = true;
	bool m_bDebugCleared = true;

	CameraRenderData m_CameraData;

	UVector2 m_v2RenderResolution = UVector2(1, 1);
	UVector2 m_v2ScreenResolution = UVector2(1, 1);

	float m_fRenderScale = 1.0f;

	uint32_t m_uiMissedFrames = 0;
	uint32_t m_uiDrawnFrames = 0;
	uint32_t m_uiFPS = 0;

	bool m_bIsDrawTextureCopied = false;
	bool m_bWorldUpdated = true;
	bool m_bCameraDataUpdated = false;
};