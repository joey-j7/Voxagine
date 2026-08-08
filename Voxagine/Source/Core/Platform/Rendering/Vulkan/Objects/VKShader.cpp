#include "pch.h"

#include "Core/Platform/Rendering/Objects/Shader.h"

#include "Core/Platform/Rendering/Vulkan/VKRenderContext.h"

#include <cstdio>
#include <fstream>

/* DX12 compiled HLSL at load time with D3DCompileFromFile. Shaders are now
   compiled to SPIR-V ahead of time by cmake/Shaders.cmake, so this only has to
   read the .spv next to the .hlsl and build a module from it. */

namespace
{
	bool ReadSpirV(const std::string& path, std::vector<uint32_t>& outWords)
	{
		std::ifstream file(path, std::ios::binary | std::ios::ate);

		if (!file.is_open())
		{
			fprintf(stderr, "[vulkan] shader not found: %s\n", path.c_str());
			return false;
		}

		const std::streamsize iSize = file.tellg();

		if (iSize <= 0 || (iSize % 4) != 0)
		{
			fprintf(stderr, "[vulkan] %s is not valid SPIR-V (%lld bytes)\n",
			        path.c_str(), static_cast<long long>(iSize));
			return false;
		}

		file.seekg(0, std::ios::beg);

		outWords.resize(static_cast<size_t>(iSize) / 4);
		file.read(reinterpret_cast<char*>(outWords.data()), iSize);

		/* SPIR-V always starts with this magic number; catches an HLSL file
		   being picked up by mistake. */
		if (outWords[0] != 0x07230203)
		{
			fprintf(stderr, "[vulkan] %s has a bad SPIR-V magic number\n", path.c_str());
			return false;
		}

		return true;
	}
}

Shader::Shader(PRenderContext* pContext, const Info& info)
{
	m_pContext = pContext;
	m_Info = info;

	m_Info.m_FilePath += ".spv";

	std::vector<uint32_t> words;

	if (!ReadSpirV(m_Info.m_FilePath, words))
		return;

	m_pNativeShader = std::make_shared<VKShaderBlob>();

	if (!m_pNativeShader->Create(pContext->GetDevice(), words))
		m_pNativeShader.reset();
}

Shader::~Shader() = default;

PShader* Shader::GetNative() const
{
	return m_pNativeShader.get();
}
