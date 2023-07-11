#pragma once

#include <string>
#include <comdef.h>
#include <assert.h>

// Helper utility converts D3D API failures into exceptions.
inline void ThrowIfShaderFailed(HRESULT hr, ID3DBlob* errorBlob)
{
	if (FAILED(hr))
	{
		_com_error err(hr);

		if (errorBlob)
		{
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			errorBlob->Release();
		}

		__debugbreak();
		//throw com_exception(hr);
	}
}

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		_com_error err(hr);

		OutputDebugString(std::string("Failure with HRESULT:  " + std::string(err.ErrorMessage()) + "\n").c_str());

		__debugbreak();
		//throw com_exception(hr);
	}
}

inline void ThrowIfFailed(HRESULT hr, ID3D12Device* pDevice)
{
	if (FAILED(hr))
	{
		_com_error err(hr);
		OutputDebugString(std::string("Failure with HRESULT:  " + std::string(err.ErrorMessage()) + "\n").c_str());

		auto h = pDevice->GetDeviceRemovedReason();
		_com_error err2(h);
		OutputDebugString(std::string("Device Removed Reason:  " + std::string(err2.ErrorMessage()) + "\n").c_str());

		__debugbreak();
		//throw com_exception(hr);
	}
}

inline UINT Align(UINT uLocation, UINT uAlign)
{
	if ((0 == uAlign) || (uAlign & (uAlign - 1)))
	{
		OutputDebugString("non-pow2 alignment");
		__debugbreak();
	}

	return ((uLocation + (uAlign - 1)) & ~(uAlign - 1));
}

#include <d3dcommon.h>
#include <iostream>
#include <fstream>

class DXShaderInclude : public ID3DInclude
{
	HRESULT Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) override
	{
		(void)pParentData;

		try {
			std::string finalPath;
			switch (IncludeType) {
			case D3D_INCLUDE_LOCAL:
				finalPath = std::string("Engine/Assets/Shaders/") + pFileName;
					break;
			case D3D_INCLUDE_SYSTEM:
				finalPath = std::string("Engine/Assets/Shaders/") + pFileName;
					break;
			default:
				assert(false);
			}

			std::ifstream includeFile(finalPath.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

			if (includeFile.is_open()) {
				long long fileSize = includeFile.tellg();
				char* buf = new char[fileSize];
				includeFile.seekg(0, std::ios::beg);
				includeFile.read(buf, fileSize);
				includeFile.close();
				*ppData = buf;
				*pBytes = static_cast<UINT>(fileSize);
			}
			else {
				return E_FAIL;
			}
			return S_OK;
		}
		catch (std::exception e) {
			return E_FAIL;
		}
	}

	HRESULT Close(LPCVOID pData) override
	{
		char* buf = (char*)pData;
		delete[] buf;
		return S_OK;
	}
};