#pragma once

#include "Core/ECS/Components/VoxRenderer.h"

class RenderContext;
class RenderSystem;
class PhysicsSystem;

class VoxelBaker
{
public:
	enum BakeType
	{
		E_OCCUPY,
		E_CLEAR
	};

	struct BakeCommand
	{
		BakeType Type;
		VoxRenderer* Renderer;
		VoxRenderer::BakeData* Data;
	};

	void Init(RenderSystem* pRenderSystem, PhysicsSystem* pPhysicsSystem);

	virtual void Bake();

	virtual uint32_t* Occupy(VoxRenderer* pRenderer, VoxRenderer::BakeData* pBakeData = nullptr);
	virtual void Clear(VoxRenderer* pRenderer, VoxRenderer::BakeData* pBakeData = nullptr);

protected:
	RenderContext* m_pRenderContext = nullptr;
	RenderSystem* m_pRenderSystem = nullptr;
	PhysicsSystem* m_pPhysicsSystem = nullptr;
};