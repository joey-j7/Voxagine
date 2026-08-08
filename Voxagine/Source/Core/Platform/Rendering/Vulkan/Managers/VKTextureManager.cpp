#include "pch.h"
#include "VKTextureManager.h"

#include "Core/Platform/Rendering/Vulkan/VKRenderContext.h"
#include "Core/Resources/Formats/TextureReference.h"

VKTextureManager::VKTextureManager(VKRenderContext* pContext) : TextureManager(pContext)
{
}

VKTextureManager::~VKTextureManager() = default;

uint32_t VKTextureManager::CreateTexture(CommandEngine* pEngine, const std::string& sName,
                                         uint8_t* pData, UVector2 uSize)
{
	VX_UNUSED(pEngine);
	VX_UNUSED(pData);

	if (uSize.x == 0 || uSize.y == 0)
		return UINT32_MAX;

	View::Info info;
	info.m_Name = sName;
	info.m_Size = UVector3(uSize.x, uSize.y, 1);
	info.m_DimensionType = E_TEXTURE_2D;
	info.m_ColorFormat = R_DEF_RESOURCE_FORMAT;
	info.m_State = E_STATE_COPY_DEST;
	info.m_Type = View::E_SHADER_RESOURCE_VIEW;

	const uint32_t uiID = m_uiNextID++;
	m_pViews[uiID] = std::make_unique<View>(m_pContext, info);

	/* Uploading pData needs a staging copy recorded on pEngine, which arrives
	   with VKView. The view exists and is correctly sized until then. */
	return uiID;
}

uint32_t VKTextureManager::CreateEmptyTexture()
{
	View::Info info;
	info.m_Name = "Empty";
	info.m_Size = UVector3(1, 1, 1);
	info.m_DimensionType = E_TEXTURE_2D;
	info.m_ColorFormat = R_DEF_RESOURCE_FORMAT;
	info.m_Type = View::E_SHADER_RESOURCE_VIEW;

	const uint32_t uiID = m_uiNextID++;
	m_pViews[uiID] = std::make_unique<View>(m_pContext, info);

	return uiID;
}

bool VKTextureManager::LoadTexture(CommandEngine* pEngine, TextureReference* pTextureReference)
{
	if (pTextureReference == nullptr)
		return false;

	VX_UNUSED(pEngine);

	/* Decoding to pixels is platform-neutral and already done above this;
	   the GPU upload lands with VKView. */
	return false;
}

void VKTextureManager::DestroyTexture(const TextureReference* pTextureReference)
{
	TextureManager::DestroyTexture(pTextureReference);
}
