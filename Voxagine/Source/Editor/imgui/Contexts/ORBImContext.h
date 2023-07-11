//#pragma once
//
//#include "ImContext.h"
//
//class ORBRenderContext;
//
//class ORBImContext : public ImContext {
//public:
//	ORBImContext(ORBRenderContext* pContext);
//
//	virtual void NewFrame() override;
//	virtual void Draw(ImDrawData* drawData, sce::Gnmx::commandl* pCommandList);
//	virtual void Deinitialize() override;
//
//protected:
//	virtual void Draw(ImDrawData* drawData) override;
//
//	ORBRenderContext* m_pContext = nullptr;
//};