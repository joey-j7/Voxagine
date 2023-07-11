#pragma once

#include "Core/Platform/Rendering/RenderContext.h"
#include "Core/VColors.h"

class Transform;
class WorldManager;

class DebugRenderer {
public:
	DebugRenderer(RenderContext* pRenderContext);

	void AddLine(const DebugLine& line);
	void AddBox(const DebugBox& box);
	void AddSphere(const DebugSphere& sphere);

	void AddLine(Vector3 start, Vector3 end, VColor color = VColors::IndianRed);

	void AddCenteredSphere(Vector3 position, Vector3 size, VColor color = VColors::IndianRed);
	void AddCenteredBox(Vector3 position, Vector3 size, VColor color = VColors::IndianRed);

	void AddCenteredLocalLine(Transform* pTransform, Vector3 size, Vector3 direction, float fDistance, VColor color = VColors::IndianRed);

private:
	RenderContext* m_pRenderContext = nullptr;
	WorldManager* m_pWorldManager = nullptr;
};