#pragma once

#include <unordered_map>
#include "Core/ECS/ComponentSystem.h"
#include "Core/Math.h"

#include "DebugRenderer.h"
#include "VoxelBaker.h"

class SpriteRenderer;
class TextRenderer;

struct Voxel;

class VoxRenderer;
class VoxAnimator;

class RenderSystem : public ComponentSystem
{
	friend class PhysicsSystem;
	friend class World;
	friend class WorldManager;
	friend class Editor;
	friend class EditorRenderMapper;
	friend class SpriteRenderer;
	friend class ParticleSystem;
	friend class VoxelBaker;

public:
	RenderSystem(World* pWorld);
	virtual ~RenderSystem();

	virtual void Start() override;

	virtual bool CanProcessComponent(Component * pComponent) override;
	virtual void Tick(float fDeltaTime) override;
	virtual void PostTick(float fDeltaTime) override;

	virtual void FixedTick(const GameTimer& fixedTimer) override;
	virtual void PostFixedTick(const GameTimer& fixedTimer) override;

	void Render(const GameTimer& fixedTimer);

	DebugRenderer& GetDebugRenderer() { return m_DebugRenderer; }

	void ForceUpdate();
	void ForceCameraDataUpdate();
	void SetGroundPlane(const std::string& texturePath, bool bForce = false);

	void EnableDebugLines(bool bEnabled);

	void SetFadeTime(float fFadeTime);

	UVector2 GetRenderResolution() const { return m_pRenderContext->GetRenderResolution(); }
	UVector2 GetScreenResolution() const { return m_pRenderContext->GetScreenResolution(); }

	uint32_t GetVoxel(uint32_t uiVolumeId) const;
	uint32_t GetVoxel(int32_t x, int32_t y, int32_t z) const;

	void Reveal() { m_bFaded = false; };
	void Fade() { m_bFaded = true; };

	bool IsFaded() const;
	bool IsFading() const;
	void SetFadeValue(float fValue) { return m_pRenderContext->SetFadeValue(fValue); }
	float GetFadeValue() const { return m_pRenderContext->m_fFader; }

protected:
	void OnWorldResumed(World* pWorld);

	virtual void OnComponentAdded(Component * pComponent) override;
	virtual void OnComponentDestroyed(Component * pComponent) override;

	inline bool ModifyVoxel(int32_t x, int32_t y, int32_t z, uint32_t uiColor, bool bOverwrite = true)
	{
		// Update world voxel
		return m_pRenderContext->ModifyVoxel(static_cast<uint32_t>(x + y * m_v3WorldSize.x + z * m_v3WorldSize.x * m_v3WorldSize.y), uiColor, bOverwrite);
	}

	inline bool ModifyVoxel(uint32_t uiVolumeId, uint32_t uiColor, bool bOverwrite = true)
	{
		// Update world voxel
		return m_pRenderContext->ModifyVoxel(uiVolumeId, uiColor, bOverwrite);
	}
	
	inline void ClearVoxels();

private:
	void CheckRendererChange(VoxRenderer* pRenderer);

	bool m_bForcedUpdate = true;
	bool m_bShouldUpdateVoxelWorld = true;
	
	UVector3 m_v3WorldSize = UVector3(0, 0, 0);
	uint32_t m_uiMaxVoxels = 0;

	bool m_bFaded = false;

	RenderContext* m_pRenderContext = nullptr;

	DebugRenderer m_DebugRenderer;

	std::vector<VoxRenderer*> m_VoxRenderers;
	std::vector<VoxAnimator*> m_VoxAnimators;

	std::vector<TextRenderer*> m_TextRenderers;
	std::vector<SpriteRenderer*> m_SpriteRenderers;

	VoxelBaker m_VoxelBaker;
	PhysicsSystem* m_pPhysicsSystem = nullptr;
};