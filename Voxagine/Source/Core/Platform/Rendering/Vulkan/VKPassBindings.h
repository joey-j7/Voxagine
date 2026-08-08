#pragma once

#include "Core/Platform/Rendering/RenderPass.h"
#include "Core/Platform/Rendering/ComputePass.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <vector>

/* Resolves a pass's resources to HLSL registers, and from there to Vulkan
 * bindings.
 *
 * The D3D12 backend assigned root parameter slots by first-seen resource name
 * (DXRenderPass::GetViewID) while the actual shader registers came from
 * separate per-type counters advancing across the resource lists in a fixed
 * order. The shaders were authored against those registers, so the order is
 * part of the contract with the HLSL and cannot be changed:
 *
 *   1. Buffers    - constant -> b, read-write -> u, otherwise -> t
 *   2. Mappers    - read-write -> u, otherwise -> t
 *   3. PassOutput - one t per render view, per pass
 *   4. Textures   - t
 *   5. Bindless   - t, and always last
 *   6. Samplers   - s, in declaration order
 *
 * Computed once and used both to build the descriptor set layout and to write
 * descriptors at draw time. Deriving those separately is how a layout and its
 * writes drift apart, and Vulkan only reports the symptom. */
struct VKPassBinding
{
	enum Kind
	{
		E_CONSTANT_BUFFER,
		E_STORAGE_BUFFER,
		E_SAMPLED_IMAGE,
		E_STORAGE_IMAGE,
		E_SAMPLER,
		E_BINDLESS_TEXTURES
	};

	Kind m_Kind = E_CONSTANT_BUFFER;

	/* What m_pSource points at. Both Buffer and Mapper can produce a storage
	   buffer binding but they are read differently, so the descriptor writer
	   cannot infer this from m_Kind. */
	enum Source
	{
		E_SOURCE_NONE,
		E_SOURCE_BUFFER,
		E_SOURCE_MAPPER,
		E_SOURCE_VIEW,
		E_SOURCE_SAMPLER
	};

	Source m_Source = E_SOURCE_NONE;

	/* HLSL register number within its class, before the VKBindings shift. */
	uint32_t m_uiRegister = 0;

	/* Vulkan binding, i.e. the register plus its class shift. */
	uint32_t m_uiBinding = 0;

	/* Greater than one only for the bindless array. */
	uint32_t m_uiCount = 1;

	VkShaderStageFlags m_Stages = 0;

	/* Whichever of Buffer/Mapper/View/Sampler produced this, or null for
	   pass outputs and bindless slots. Not owned. */
	const void* m_pSource = nullptr;

	std::string m_Name;
};

/* Order matches DXRenderPass::Init exactly. */
std::vector<VKPassBinding> VKBuildRenderPassBindings(const RenderPass::Data& data);

std::vector<VKPassBinding> VKBuildComputePassBindings(const ComputePass::Data& data);

/* Feeds the result into a VKDescriptorLayout. Separate so the binding table
   can be inspected and asserted against without building anything. */
class VKDescriptorLayout;
bool VKApplyBindings(VKDescriptorLayout& layout, const std::vector<VKPassBinding>& bindings);
