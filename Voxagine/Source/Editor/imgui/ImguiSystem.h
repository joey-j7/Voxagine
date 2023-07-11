#pragma once

class ImPlatform;
class ImContext;
class RenderContext;
class Application;

struct ID3D12GraphicsCommandList;

#define NUM_FRAMES_IN_FLIGHT 3

class ImguiSystem {
public:
	void Initialize(RenderContext* pRenderContext);
	void Update();
	void Draw();

	void Deinitialize();

	ImPlatform* GetPlatform() const { return m_pPlatform; }
	void SetPlatform(ImPlatform* pPlatform) { m_pPlatform = pPlatform; };

	ImContext* GetContext() const { return m_pContext; }
	void SetContext(ImContext* pContext) { m_pContext = pContext; };

protected:
	void InitStyle();

	RenderContext* m_pRenderContext = nullptr;

	ImPlatform* m_pPlatform = nullptr;
	ImContext* m_pContext = nullptr;
};