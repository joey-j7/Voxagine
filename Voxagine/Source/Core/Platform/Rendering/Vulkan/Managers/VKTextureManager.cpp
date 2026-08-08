#include "pch.h"
#include "VKTextureManager.h"

#include "Core/Platform/Rendering/Vulkan/VKCommandEngine.h"
#include "Core/Platform/Rendering/Vulkan/VKRenderContext.h"
#include "Core/Resources/Formats/TextureReference.h"

#include <cstring>

VKTextureManager::VKTextureManager(VKRenderContext* pContext) : TextureManager(pContext)
{
}

VKTextureManager::~VKTextureManager() = default;

uint32_t VKTextureManager::CreateTexture(CommandEngine* pEngine, const std::string& sName,
                                         uint8_t* pData, UVector2 uSize)
{
	if (uSize.x == 0 || uSize.y == 0)
		return UINT32_MAX;

	View::Info info;
	info.m_Name = sName;
	info.m_Size = UVector3(uSize.x, uSize.y, 1);
	info.m_DimensionType = E_TEXTURE_2D;
	/* UNORM, not sRGB: the DX12 backend sampled sprites as UNORM and the
	   post-processing chain is tuned around that. */
	info.m_ColorFormat = E_R8G8B8A8_UNORM;
	info.m_State = E_STATE_COPY_DEST;
	info.m_Type = View::E_SHADER_RESOURCE_VIEW;

	const uint32_t uiID = m_uiNextID++;
	m_pViews[uiID] = std::make_unique<View>(m_pContext, info);

	View* pView = m_pViews[uiID].get();

	if (pView->GetNative() == nullptr)
		return UINT32_MAX;

	if (pData == nullptr || pEngine == nullptr)
		return uiID;

	VKRenderContext* pContext = m_pContext;
	PCommandEngine* pVKEngine = pEngine->Get();

	const VkDeviceSize uiBytes =
		static_cast<VkDeviceSize>(uSize.x) * uSize.y * sizeof(uint32_t);

	/* Staging buffer; Vulkan needs no row alignment when bufferRowLength is 0. */
	VKResource staging;

	if (!staging.CreateBuffer(pContext->GetDevice(), pContext->GetAllocator(), uiBytes,
	                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
	{
		fprintf(stderr, "[vulkan] staging buffer for '%s' failed\n", sName.c_str());
		return uiID;
	}

	void* pMapped = staging.Map();

	if (pMapped == nullptr)
	{
		staging.Destroy();
		return uiID;
	}

	std::memcpy(pMapped, pData, uiBytes);

	/* Load-time upload on the dedicated Texture engine; blocking, exactly as
	   the DX12 path was. */
	pVKEngine->Reset();
	pVKEngine->Start();

	pVKEngine->QueueBarrier(pView->GetNative(), E_STATE_COPY_DEST);
	pVKEngine->ApplyBarriers();

	VkBufferImageCopy region{};
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent = { uSize.x, uSize.y, 1 };

	vkCmdCopyBufferToImage(pVKEngine->GetCommandBuffer(), staging.GetBuffer(),
	                       pView->GetNative()->GetImage(),
	                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	pVKEngine->QueueBarrier(pView->GetNative(), E_STATE_PIXEL_SHADER_RESOURCE);

	pVKEngine->Execute();
	pVKEngine->WaitForGPU();

	staging.Destroy();

	return uiID;
}

uint32_t VKTextureManager::CreateEmptyTexture()
{
	View::Info info;
	info.m_Name = "Empty";
	info.m_Size = UVector3(1, 1, 1);
	info.m_DimensionType = E_TEXTURE_2D;
	info.m_ColorFormat = E_R8G8B8A8_UNORM;
	info.m_Type = View::E_SHADER_RESOURCE_VIEW;

	const uint32_t uiID = m_uiNextID++;
	m_pViews[uiID] = std::make_unique<View>(m_pContext, info);

	return uiID;
}

bool VKTextureManager::LoadTexture(CommandEngine* pEngine, TextureReference* pTextureReference)
{
	if (pTextureReference == nullptr)
		return false;

	const std::string& sPath = pTextureReference->GetRefPath();
	TextureReadData* pTextureData = ReadTexture(sPath);

	if (pTextureData == nullptr || pTextureData->m_Data == nullptr)
	{
		delete pTextureData;
		return false;
	}

	const uint32_t uiID = CreateTexture(pEngine, sPath,
	                                    reinterpret_cast<uint8_t*>(pTextureData->m_Data),
	                                    pTextureData->m_Dimensions);
	delete pTextureData;

	if (uiID == UINT32_MAX)
		return false;

	pTextureReference->m_uiID = uiID;
	pTextureReference->TextureView = m_pViews[uiID].get();

	return true;
}

void VKTextureManager::DestroyTexture(const TextureReference* pTextureReference)
{
	TextureManager::DestroyTexture(pTextureReference);
}
