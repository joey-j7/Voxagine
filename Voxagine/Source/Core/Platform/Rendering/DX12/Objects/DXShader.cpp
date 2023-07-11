#include "pch.h"
#include "Core/Platform/Rendering/Objects/Shader.h"

#include "Core/Platform/Rendering/DX12/DXHelper.h"
#include <d3dcompiler.h>

#include "Core/Platform/Rendering/DX12/DX12RenderContext.h"
#include "Core/System/FileSystem.h"
#include "Core/Platform/Platform.h"
#include "Core/Application.h"

#include <xlocbuf>
#include <codecvt>

Shader::Shader(PRenderContext* pContext, const Info& info)
{
	m_Info = info;

	m_Info.m_FilePath += ".hlsl";

#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;
#else
	UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;
#endif

	ID3DBlob* errorBlob = nullptr;
	DXShaderInclude inc;

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring filePath = converter.from_bytes(m_Info.m_FilePath);

	ThrowIfShaderFailed(
		D3DCompileFromFile(
			filePath.c_str(),
			nullptr,
			&inc,
			"main",
			info.m_Type == E_PIXEL ? "ps_5_1" : info.m_Type == E_VERTEX ? "vs_5_1" : "cs_5_1",
			compileFlags,
			0,
			&m_pNativeShader,
			&errorBlob
		),
		errorBlob
	);
}

Shader::~Shader() {}

PShader* Shader::GetNative() const { return m_pNativeShader.Get(); }