#pragma once

#include "Core/Platform/Rendering/RenderDefines.h"
#include "Core/Platform/Rendering/Vulkan/VKAllocator.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <vector>

class VKDevice;

/* Backs PResource / PTexture.
 *
 * DX12 had one ID3D12Resource for images and buffers alike, and the engine's
 * Buffer/View/Mapper are written against that single type. Vulkan needs
 * VkImage and VkBuffer to be different objects, so this holds whichever one
 * applies and keeps the engine's one-handle assumption intact.
 *
 * m_State tracks the last transition so barriers can be emitted without the
 * caller having to remember the previous layout - the DX12 backend leaned on
 * fixed per-pass rules for this, which is what made a frame graph impossible
 * to add later. */
class VKResource
{
public:
	enum Kind
	{
		E_KIND_NONE,
		E_KIND_IMAGE,
		E_KIND_BUFFER
	};

	VKResource() = default;
	~VKResource();

	VKResource(const VKResource&) = delete;
	VKResource& operator=(const VKResource&) = delete;

	bool CreateImage(VKDevice* pDevice, const VKAllocator* pAllocator,
	                 VkImageType type, VkFormat format,
	                 uint32_t uiWidth, uint32_t uiHeight, uint32_t uiDepth,
	                 uint32_t uiMipLevels, VkImageUsageFlags usage);

	bool CreateBuffer(VKDevice* pDevice, const VKAllocator* pAllocator,
	                  VkDeviceSize uiSize, VkBufferUsageFlags usage,
	                  VkMemoryPropertyFlags properties);

	void Destroy();

	/* Host-visible resources only; returns nullptr otherwise. */
	void* Map();
	void Unmap();

	/* Emits the barrier needed to move from the tracked state to newState and
	   records it as current. No-op when already in newState. */
	void Transition(VkCommandBuffer cmd, PEResourceState newState);

	Kind GetKind() const { return m_Kind; }

	VkImage GetImage() const { return m_Image; }
	VkBuffer GetBuffer() const { return m_Buffer; }
	VkFormat GetFormat() const { return m_Format; }
	VkExtent3D GetExtent() const { return m_Extent; }

	/* Lazily created and cached. Vulkan needs a VkImageView wherever D3D12
	   would have taken the resource directly. */
	VkImageView GetOrCreateImageView(VkImageViewType type);

	VkDeviceSize GetSize() const { return m_Allocation.m_uiSize; }
	PEResourceState GetState() const { return m_State; }

	/* True until the first transition. A caller that skips barriers on
	   matching state must still emit one while this holds. */
	bool IsLayoutUndefined() const { return m_bLayoutUndefined; }

	void SetDebugName(const std::string& name) { m_Name = name; }
	const std::string& GetDebugName() const { return m_Name; }

private:
	VKDevice* m_pDevice = nullptr;
	const VKAllocator* m_pAllocator = nullptr;

	Kind m_Kind = E_KIND_NONE;

	VkImage m_Image = VK_NULL_HANDLE;
	VkBuffer m_Buffer = VK_NULL_HANDLE;
	VkFormat m_Format = VK_FORMAT_UNDEFINED;
	VkExtent3D m_Extent = { 0, 0, 0 };
	VkImageView m_ImageView = VK_NULL_HANDLE;

	VKAllocator::Allocation m_Allocation;

	PEResourceState m_State = E_STATE_COMMON_RESOURCE;
	bool m_bIsDepth = false;

	/* A freshly created image is genuinely in VK_IMAGE_LAYOUT_UNDEFINED, which
	   no PEResourceState can express - the engine's vocabulary has no
	   equivalent because D3D12 had no such concept. Tracked separately so the
	   first transition uses UNDEFINED as its old layout instead of whatever
	   m_State claims. */
	bool m_bLayoutUndefined = false;

	void* m_pMapped = nullptr;
	std::string m_Name;
};

/* Backs PVShader / PFShader / PShader. SPIR-V words plus the module built
   from them; DX12 kept an ID3DBlob of DXBC here. */
class VKShaderBlob
{
public:
	VKShaderBlob() = default;
	~VKShaderBlob();

	bool Create(VKDevice* pDevice, const std::vector<uint32_t>& a_SpirV);
	void Destroy();

	VkShaderModule GetModule() const { return m_Module; }
	const std::vector<uint32_t>& GetSpirV() const { return m_SpirV; }

	bool IsValid() const { return m_Module != VK_NULL_HANDLE; }

private:
	VKDevice* m_pDevice = nullptr;
	VkShaderModule m_Module = VK_NULL_HANDLE;
	std::vector<uint32_t> m_SpirV;
};

/* Backs PCommandList. A thin wrapper so the engine can keep passing a
   pointer-to-command-list around the way it did with ID3D12GraphicsCommandList. */
class VKCommandList
{
public:
	VKCommandList() = default;
	explicit VKCommandList(VkCommandBuffer buffer) : m_Buffer(buffer) {}

	VkCommandBuffer Get() const { return m_Buffer; }
	void Set(VkCommandBuffer buffer) { m_Buffer = buffer; }

	bool IsValid() const { return m_Buffer != VK_NULL_HANDLE; }

private:
	VkCommandBuffer m_Buffer = VK_NULL_HANDLE;
};
