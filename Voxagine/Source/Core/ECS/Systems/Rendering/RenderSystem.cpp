#include "pch.h"
#include "RenderSystem.h"

#include <algorithm>

#include "Core/ECS/Components/SpriteRenderer.h"
#include "Core/ECS/Components/TextRenderer.h"
#include "Core/ECS/Components/VoxRenderer.h"

#include "Core/ECS/World.h"
#include "Core/ECS/ComponentSystem.h"
#include "Core/ECS/Entities/Camera.h"

#include "Core/ECS/Systems/Physics/PhysicsSystem.h"
#include "Core/Platform/Rendering/RenderAlignment.h"

#include "Core/Platform/Platform.h"
#include "Core/Application.h"

#include "Core/ECS/Systems/Rendering/Buffers/RenderData.h"

#include "Core/Platform/Rendering/RenderContext.h"
#include "Core/Settings.h"
#include "Core/GameTimer.h"

// For Font Glyphs
#include <External/imgui/imgui.h>

#if defined(EDITOR) || defined(_DEBUG)
#include "../../Component.h"
#include "../../Components/Transform.h"
#include <Core/Resources/Formats/VoxModel.h>
#endif

#include "../../Components/VoxAnimator.h"
#include "DebugRenderer.h"

RenderSystem::RenderSystem(World* pWorld) :
	ComponentSystem(pWorld),
	m_pRenderContext(m_pWorld->GetApplication()->GetPlatform().GetRenderContext()),
	m_DebugRenderer(m_pRenderContext)
{
	m_pPhysicsSystem = m_pWorld->GetSystem<PhysicsSystem>();
	m_pPhysicsSystem->SetRenderSystem(this);

	m_VoxelBaker.Init(this, m_pPhysicsSystem);

	/* Prepare color data */
	VoxelGrid* pVoxelGrid = m_pPhysicsSystem->GetVoxelGrid();
	pVoxelGrid->GetDimensions(m_v3WorldSize.x, m_v3WorldSize.y, m_v3WorldSize.z);

	m_uiMaxVoxels = m_v3WorldSize.x * m_v3WorldSize.y * m_v3WorldSize.z;

	m_pWorld->Resumed += Event<World*>::Subscriber(std::bind(&RenderSystem::OnWorldResumed, this, std::placeholders::_1), this);

	// Temp for compilation purposes
	Entity entity(pWorld);
	TextRenderer textRenderer(&entity);
}

RenderSystem::~RenderSystem()
{
	for (VoxRenderer* pRenderer : m_VoxRenderers)
	{
		/* Remove old voxels if array is valid */
		if (pRenderer->m_BakeData.Positions)
		{
			delete[] pRenderer->m_BakeData.Positions;
			pRenderer->m_BakeData.Positions = nullptr;
		}
	}

	m_pRenderContext->SetFadeValue(1.f);

	/* Clear the current world's voxels */
	ClearVoxels();

	m_pWorld->Resumed -= this;
}

void RenderSystem::Start()
{
	if (!m_pRenderContext->ResizeWorldBuffer())
		ClearVoxels();

	SetGroundPlane(m_pWorld->GetGroundTexturePath(), true);

	if (!m_pWorld->GetApplication()->IsInEditor())
		m_pRenderContext->SetFadeValue(0.f);

	m_pRenderContext->m_fFadeTime = 1.f;

	ForceUpdate();
}

bool RenderSystem::CanProcessComponent(Component* pComponent)
{
	return dynamic_cast<VoxRenderer*>(pComponent) || dynamic_cast<VoxAnimator*>(pComponent) || dynamic_cast<SpriteRenderer*>(pComponent) || dynamic_cast<TextRenderer*>(pComponent);
}

void RenderSystem::Tick(float fDeltaTime)
{
}

void RenderSystem::PostTick(float fDeltaTime)
{
	/* Submit voxel data */
	m_pRenderContext->Submit(RenderData(
		nullptr,
		m_v3WorldSize.x * m_v3WorldSize.y * m_v3WorldSize.z * sizeof(uint32_t),
		{ sizeof(uint32_t) },
		RenderDataType::VOXEL
	));

	for (VoxRenderer* pRenderer : m_VoxRenderers) {
		CheckRendererChange(pRenderer);

		const VoxFrame* pFrame = pRenderer->GetFrame();
		if (pFrame == nullptr || !pRenderer->IsEnabled()) continue;

		Box bounds = pRenderer->GetBounds();
		Vector3 offset = Vector3(0.f, 0.f, 0.f);
		if (pFrame->GetModel()->GetFrameCount() > 1) {//Animation

			//Get animation relative-to-absolute offset
			const VoxFrame* tFrame = pFrame->GetModel()->GetFrame(0);
			Vector3 offsetCen = -(tFrame->GetFitSizeOffset() - pFrame->GetFitSizeOffset()) * 0.5f
				+ ((pFrame->GetFitSizeOffset() + pFrame->GetFittedSize()) - (tFrame->GetFitSizeOffset() + tFrame->GetFittedSize())) * 0.5f;
			offsetCen.y *= -1.f;

			//Transform the offset based on rotation and scale
			offset = pRenderer->GetTransform()->GetMatrix() * Vector4(offsetCen, 1.f);
			offset -= pRenderer->GetTransform()->GetPosition();
		}

		StructuredVoxelBuffer buffer;
		buffer.Position = m_pPhysicsSystem->m_VoxelGrid.WorldToGrid(pRenderer->GetTransform()->GetPosition()-offset, true);
		buffer.Extents = bounds.GetSize() * 0.5f + Vector3(1.f);
		buffer.MapperID = pRenderer->GetFrame()->GetMapperID();

		m_pRenderContext->Submit(buffer);

#if defined(EDITOR) || defined(_DEBUG)
		if (!pRenderer->DrawBoundsEnabled())
			continue;

		DebugBox box;
		box.m_Center = pRenderer->GetTransform()->GetPosition()-offset;
		box.m_Extents = bounds.GetSize() * 0.5f;
		box.m_Color = VColors::LightSkyBlue;

		m_DebugRenderer.AddBox(
			box
		);
#endif
	}

#ifndef _ORBIS
	/* Render text data */
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.f, 0.f));

	uint32_t uiTextRendererCount = 0;

	for (TextRenderer* pTextRenderer : m_TextRenderers)
	{
		if (!pTextRenderer->IsEnabled() || pTextRenderer->GetText().empty())
			continue;

		Vector3 position = pTextRenderer->GetTransform()->GetPosition();

		float scale = pTextRenderer->GetScale();
		Vector2 screenScale = Vector2(1.f, 1.f);

		if (pTextRenderer->ScalesWithScreen())
		{
			Vector2 renderRes = m_pRenderContext->GetRenderResolution();
			screenScale = Vector2(renderRes.x / 1280.f, renderRes.y / 720.f);
			scale *= std::min(screenScale.x, screenScale.y);
		}

		Vector2 v2Alignment = GetNormRenderAlignment(pTextRenderer->GetAlignment());
		Vector2 v2ScreenAlignment = GetNormRenderAlignment(pTextRenderer->GetScreenAlignment());

		ImVec2 alignment = *(ImVec2*)&v2Alignment;
		ImVec2 screenAlignment = *(ImVec2*)&v2ScreenAlignment;

		ImGui::SetNextWindowBgAlpha(0.f);
		
		ImGui::Begin(("Text Renderer " + std::to_string(uiTextRendererCount)).c_str(), NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs);
		uiTextRendererCount++;

		ImGui::PushStyleColor(
			ImGuiCol_Text,
			ImVec4(
				pTextRenderer->GetColor().inst.Colors.r / 255.f,
				pTextRenderer->GetColor().inst.Colors.g / 255.f,
				pTextRenderer->GetColor().inst.Colors.b / 255.f,
				pTextRenderer->GetColor().inst.Colors.a / 255.f * m_pRenderContext->GetFadeValue()
			)
		);

		ImGui::SetWindowFontScale(scale);

		ImVec2 windowPosition = ImVec2(
			screenAlignment.x * ImGui::GetIO().DisplaySize.x + position.x * screenScale.x - ImGui::GetWindowWidth() * alignment.x,
			screenAlignment.y * ImGui::GetIO().DisplaySize.y - position.y * screenScale.y - ImGui::GetWindowHeight() * alignment.y
		);

		if (pTextRenderer->IsWrapping())
		{
			ImGui::PushTextWrapPos(ImGui::GetIO().DisplaySize.x - windowPosition.x);
			ImGui::TextWrapped(pTextRenderer->GetText().c_str());
			ImGui::PopTextWrapPos();
		}
		else
		{
			ImGui::Text(pTextRenderer->GetText().c_str());
		}

		ImGui::SetWindowPos(windowPosition);

		ImGui::PopStyleColor();
		ImGui::End();
	}

	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
#endif

	// Sort AABBs
	m_pRenderContext->SortAABBs();

	/* Ground plane SDF */
	DebugBox box;
	box.m_Extents = Vector3((float)m_v3WorldSize.x, 5.0f, (float)m_v3WorldSize.z) * 0.5f;
	box.m_Center = m_pPhysicsSystem->m_VoxelGrid.GridToWorld(box.m_Extents);
	box.m_Color = VColors::LightSkyBlue;

	StructuredVoxelBuffer buffer;
	buffer.Position = m_pPhysicsSystem->m_VoxelGrid.WorldToGrid(box.m_Center, true);
	buffer.Extents = box.m_Extents;
	buffer.MapperID = 0;

	m_pRenderContext->Submit(buffer);

#if defined(EDITOR) || defined(_DEBUG)
	m_DebugRenderer.AddBox(
		box
	);
#endif
}

void RenderSystem::FixedTick(const GameTimer& fixedTimer)
{
	// Fade scene
	if (m_bFaded)
	{
		if (m_pRenderContext->m_fFader > 0.f)
		{
			m_pRenderContext->SetFadeValue(
				std::max(
					0.f,
					m_pRenderContext->m_fFader - static_cast<float>(fixedTimer.GetElapsedSeconds()) / m_pRenderContext->m_fFadeTime
				)
			);
		}
	}
	else
	{
		if (m_pRenderContext->m_fFader < 1.f)
		{
			m_pRenderContext->SetFadeValue(
				std::min(
					1.f,
					m_pRenderContext->m_fFader + static_cast<float>(fixedTimer.GetElapsedSeconds()) / m_pRenderContext->m_fFadeTime
				)
			);
		}
	}

	/* Update animators on a fixed timestep */
	for (auto& pAnimator : m_VoxAnimators) {
		if (!pAnimator->IsEnabled())
			continue;

		pAnimator->Tick(static_cast<float>(fixedTimer.GetElapsedSeconds()));
	}
}

void RenderSystem::PostFixedTick(const GameTimer& fixedTimer)
{
	m_pRenderContext->FixedClear();

	/* Submit sprite data */
	SpriteData spriteData;

	// Sort sprite renders by layer, so the rendering is updated in the editor when changing the layer
	std::map<int, std::vector<SpriteRenderer*>, std::less<int>> LayeredSpriteRenderers;
	for (SpriteRenderer* pSpriteRenderer : m_SpriteRenderers)
	{
		if (pSpriteRenderer)
			LayeredSpriteRenderers[pSpriteRenderer->GetRenderLayer()].push_back(pSpriteRenderer);
	}

	for (std::pair<int, std::vector<SpriteRenderer*>> LayeredSpriteRenderer : LayeredSpriteRenderers)
	{
		for (SpriteRenderer* pSpriteRenderer : LayeredSpriteRenderer.second)
		{
			if (
				!pSpriteRenderer ||
				!pSpriteRenderer->IsEnabled() ||
				!pSpriteRenderer->m_pTextureReference ||
				!pSpriteRenderer->m_pTextureReference->IsLoaded() ||
				!pSpriteRenderer->m_pTextureReference->TextureView
			)
				continue;

			if (!pSpriteRenderer->IsScreenSpace() && pSpriteRenderer->IsBillboard())
			{
				pSpriteRenderer->GetTransform()->SetRotation(m_pWorld->GetMainCamera()->GetTransform()->GetRotation());
			}

			Vector2 scale = pSpriteRenderer->GetScale();
			float minScale = std::min(scale.x, scale.y);

			spriteData.Model = pSpriteRenderer->GetTransform()->GetMatrix();
			spriteData.Model *= glm::scale(Vector3(minScale, minScale, 1.f));

			spriteData.Model[3][0] = pSpriteRenderer->GetTransform()->GetPosition().x * scale.x;
			spriteData.Model[3][1] = pSpriteRenderer->GetTransform()->GetPosition().y * scale.y;

			spriteData.TextureID = pSpriteRenderer->m_pTextureReference->GetID();

			VColor color = pSpriteRenderer->GetColor();

			spriteData.Color = Vector4(
				color.inst.Colors.r / 255.0,
				color.inst.Colors.g / 255.0,
				color.inst.Colors.b / 255.0,
				color.inst.Colors.a / 255.0
			);

			spriteData.Size = pSpriteRenderer->m_pTextureReference->TextureView->GetInfo().m_Size;

			spriteData.Alignment = pSpriteRenderer->GetAlignment();
			spriteData.ScreenAlignment = pSpriteRenderer->GetScreenAlignment();

			spriteData.IsScreen = pSpriteRenderer->IsScreenSpace();

			spriteData.Layer = pSpriteRenderer->GetRenderLayer();

			spriteData.TextureRepeat = pSpriteRenderer->GetTilingAmount();

			spriteData.cullStart = pSpriteRenderer->GetCullingStart();
			spriteData.cullEnd = pSpriteRenderer->GetCullingEnd();

			m_pRenderContext->Submit(spriteData);
		}
	}
}

void RenderSystem::Render(const GameTimer& fixedTimer)
{
	/* Get voxel data on fixed timestep */
	bool bShouldUpdateVoxelWorld = m_bForcedUpdate;
	Camera* pCamera = m_pWorld->GetMainCamera();

	m_bShouldUpdateVoxelWorld = bShouldUpdateVoxelWorld || pCamera->IsUpdated();

	m_VoxelBaker.Bake();

	if (bShouldUpdateVoxelWorld)
		m_pRenderContext->ForceUpdate();

	m_bForcedUpdate = false;
}

void RenderSystem::OnWorldResumed(World* pWorld)
{
	/* Clear voxel world, make planes and force update */

	if (!m_pRenderContext->ResizeWorldBuffer())
		ClearVoxels();

	if (!m_pWorld->GetApplication()->IsInEditor())
		m_pRenderContext->SetFadeValue(0.f);

	m_pRenderContext->m_fFadeTime = 1.f;

	

	//ForceUpdate();
}

void RenderSystem::OnComponentAdded(Component* pComponent)
{
	if (VoxRenderer* pRenderer = dynamic_cast<VoxRenderer*>(pComponent))
	{
		pRenderer->GetOwner()->StaticPropertyChanged -= this;
		pRenderer->GetOwner()->StaticPropertyChanged += Event<Entity*, bool>::Subscriber([this](Entity* pEntity, bool isStatic)
		{
			VoxRenderer* pRenderer = pEntity->GetComponent<VoxRenderer>();
			if (pRenderer)
			{
				OnComponentDestroyed(pRenderer);
				OnComponentAdded(pRenderer);
				pRenderer->RequestUpdate();
			}
		}, this);

		// Set render data and occupy space in world
		Vector3 pos = pRenderer->GetTransform()->GetPosition();
		pos.x = floor(pos.x);
		pos.y = floor(pos.y);
		pos.z = floor(pos.z);

		VoxRenderer::BakeData bakeData;

		bakeData.LastLocation = pos;
		bakeData.LastRotation = pRenderer->GetTransform()->GetRotation();
		bakeData.LastScale = pRenderer->GetTransform()->GetScale();
		bakeData.WorldOffset = m_pPhysicsSystem->m_VoxelGrid.GetWorldOffset();
		bakeData.IsEnabled = pRenderer->IsEnabled();

		m_VoxRenderers.push_back(pRenderer);
		pRenderer->m_BakeData = bakeData;

		if (!pRenderer->IsChunkInstanceLoaded())
		{
			uint32_t* voxels = m_VoxelBaker.Occupy(pRenderer, &pRenderer->m_BakeData);
			pRenderer->m_BakeData.Positions = voxels;
		}
	}
	else if (VoxAnimator* pAnimator = dynamic_cast<VoxAnimator*>(pComponent))
	{
		m_VoxAnimators.push_back(pAnimator);
	}
	else if (TextRenderer* pText = dynamic_cast<TextRenderer*>(pComponent))
	{
		m_TextRenderers.push_back(pText);
		pText->m_pRenderSystem = this;
	}
	else if (SpriteRenderer* pSprite = dynamic_cast<SpriteRenderer*>(pComponent))
	{
		m_SpriteRenderers.push_back(pSprite);
		pSprite->m_pRenderSystem = this;
	}
}

void RenderSystem::OnComponentDestroyed(Component* pComponent)
{
	if (VoxRenderer* pVoxRenderer = dynamic_cast <VoxRenderer*>(pComponent))
	{
		auto iter = std::find(m_VoxRenderers.begin(), m_VoxRenderers.end(), pVoxRenderer);

		if (iter != m_VoxRenderers.end())
		{
			/* Remove old voxels if array is valid */
			m_VoxelBaker.Clear(*iter);
			m_VoxRenderers.erase(iter);
		}
	}
	else if (VoxAnimator* pVoxAnimator = dynamic_cast <VoxAnimator*>(pComponent))
	{
		m_VoxAnimators.erase(std::remove(m_VoxAnimators.begin(), m_VoxAnimators.end(), pVoxAnimator), m_VoxAnimators.end());
	}
	else if (TextRenderer* pText = dynamic_cast<TextRenderer*>(pComponent))
	{
		m_TextRenderers.erase(std::remove(m_TextRenderers.begin(), m_TextRenderers.end(), pText), m_TextRenderers.end());
	}
	else if (SpriteRenderer* pSprite = dynamic_cast<SpriteRenderer*>(pComponent))
	{
		m_SpriteRenderers.erase(std::remove(m_SpriteRenderers.begin(), m_SpriteRenderers.end(), pSprite), m_SpriteRenderers.end());
	}
}

uint32_t RenderSystem::GetVoxel(int32_t x, int32_t y, int32_t z) const
{
	if (x < 0 || y < 0 || z < 0)
		return 0;

	return m_pRenderContext->GetVoxel(static_cast<uint32_t>(x + y * m_v3WorldSize.x + z * m_v3WorldSize.x * m_v3WorldSize.y));
}

uint32_t RenderSystem::GetVoxel(uint32_t uiVolumeId) const
{
	return m_pRenderContext->GetVoxel(uiVolumeId);
}

void RenderSystem::ClearVoxels()
{
	m_pRenderContext->ClearVoxels();
}

bool RenderSystem::IsFaded() const
{
	return m_pRenderContext->m_fFader <= 0.f;
}

bool RenderSystem::IsFading() const
{
	return m_pRenderContext->m_fFader > 0.f && m_pRenderContext->m_fFader < 1.f;
}

void RenderSystem::ForceUpdate()
{
	m_bForcedUpdate = true;
	m_pRenderContext->ForceUpdate();
}

void RenderSystem::ForceCameraDataUpdate()
{
	m_pRenderContext->ForceCameraDataUpdate();
}

void RenderSystem::SetGroundPlane(const std::string& texturePath, bool bForce)
{
	VoxelGrid* pVoxelGrid = m_pPhysicsSystem->GetVoxelGrid();
	TextureReadData* pTextureData = nullptr;
	
	if (!texturePath.empty())
		pTextureData = m_pWorld->GetApplication()->GetPlatform().GetRenderContext()->ReadTexture(texturePath);

	uint32_t id = 0;
	uint32_t color = VColor(static_cast<unsigned char>(50), 50, 50, 255).inst.Color;

	bool bHasData = pTextureData && pTextureData->m_Data && pTextureData->m_Dimensions.x > 0 && pTextureData->m_Dimensions.y > 0;

	for (uint32_t z = 0; z < m_v3WorldSize.z; ++z)
	{
		for (uint32_t x = 0; x < m_v3WorldSize.x; ++x)
		{
			if (bHasData)
			{
				id = x % pTextureData->m_Dimensions.x + ((pTextureData->m_Dimensions.y - 1 - z) * pTextureData->m_Dimensions.x) % (pTextureData->m_Dimensions.x * (pTextureData->m_Dimensions.y));
				color = pTextureData->m_Data[id];
			}

			ModifyVoxel(
				x, 0, z,
				color
			);

			Voxel* pVoxel = pVoxelGrid->GetVoxel(x, 0, z);
			pVoxel->Active = true;
			pVoxel->Color = color;
			pVoxel->UserPointer = 0;
		}
	}

	delete pTextureData;
}

void RenderSystem::EnableDebugLines(bool bEnabled)
{
	m_pRenderContext->EnableDebugLines(bEnabled);
}

void RenderSystem::SetFadeTime(float fFadeTime)
{
	m_pRenderContext->m_fFadeTime = fFadeTime <= 0.0f ? 1.0f : fFadeTime;
}

void RenderSystem::CheckRendererChange(VoxRenderer* pRenderer)
{
	if (pRenderer->IsEnabled() != pRenderer->m_BakeData.IsEnabled)
	{
		pRenderer->m_BakeData.Updated = true;
		pRenderer->m_BakeData.IsEnabled = pRenderer->IsEnabled();
		return;
	}
	
	if (pRenderer->m_BakeData.Updated)
		return;

	if (pRenderer->IsFrameChanged())
	{
		pRenderer->ResetFrameChanged();
		pRenderer->m_BakeData.Updated = true;
		return;
	}

	Transform* pTransform = pRenderer->GetTransform();
	Vector3 scale = pTransform->GetScale();

	if (
		scale.x != pRenderer->m_BakeData.LastScale.x ||
		scale.y != pRenderer->m_BakeData.LastScale.y ||
		scale.z != pRenderer->m_BakeData.LastScale.z
		)
	{
		pRenderer->m_BakeData.Updated = true;
		return;
	}

	Vector3 position = pTransform->GetPosition();
	//Vector3 position = m_pPhysicsSystem->m_VoxelGrid.WorldToGrid(pTransform->GetPosition());

	if (
		glm::distance(pRenderer->m_BakeData.LastLocation.x, floor(position.x)) >= 1.0f ||
		glm::distance(pRenderer->m_BakeData.LastLocation.y, floor(position.y)) >= 1.0f ||
		glm::distance(pRenderer->m_BakeData.LastLocation.z, floor(position.z)) >= 1.0f
	)
	{
		pRenderer->m_BakeData.Updated = true;
		return;
	}

	Quaternion rotation = pTransform->GetRotation();

	if (
		pRenderer->m_BakeData.LastRotation.x != rotation.x ||
		pRenderer->m_BakeData.LastRotation.y != rotation.y ||
		pRenderer->m_BakeData.LastRotation.z != rotation.z ||
		pRenderer->m_BakeData.LastRotation.w != rotation.w
	)
	{
		pRenderer->m_BakeData.Updated = true;
	}
}
