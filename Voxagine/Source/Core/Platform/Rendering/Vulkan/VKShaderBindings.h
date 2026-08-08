#pragma once

#include <cstdint>

/* HLSL register class -> Vulkan binding offsets.
 *
 * HLSL has four independent register namespaces (b, t, u, s); Vulkan has one
 * binding namespace per descriptor set. Compiled without shifts, a shader
 * declaring both b0 and u0 produces two descriptors at set 0 binding 0, which
 * is an invalid descriptor set layout - VoxelRenderer.ps.hlsl does exactly
 * that. DXC's -fvk-*-shift options move each class into a disjoint range.
 *
 * These MUST match the shifts passed in cmake/Shaders.cmake. They are the
 * contract between the compiled SPIR-V and the descriptor set layouts built
 * on the C++ side. */
namespace VKBindings
{
	static const uint32_t k_uiConstantBufferOffset = 0;   // b
	static const uint32_t k_uiTextureOffset        = 100; // t
	static const uint32_t k_uiUnorderedOffset      = 200; // u
	static const uint32_t k_uiSamplerOffset        = 300; // s

	/* Register classes are spaced this far apart, so a shader may use up to
	   this many registers of one class before colliding with the next. */
	static const uint32_t k_uiClassStride = 100;

	inline uint32_t ConstantBuffer(uint32_t uiRegister) { return k_uiConstantBufferOffset + uiRegister; }
	inline uint32_t Texture(uint32_t uiRegister)        { return k_uiTextureOffset + uiRegister; }
	inline uint32_t Unordered(uint32_t uiRegister)      { return k_uiUnorderedOffset + uiRegister; }
	inline uint32_t Sampler(uint32_t uiRegister)        { return k_uiSamplerOffset + uiRegister; }
}
