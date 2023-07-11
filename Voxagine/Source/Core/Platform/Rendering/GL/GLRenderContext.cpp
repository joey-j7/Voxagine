#include "pch.h"
//#include "GLRenderContext.h"
//
//#include "../../Platform.h"
//#include "../../Window/WindowContext.h"
//
//#include "GLHelper.h"
//
//#include "../RenderContext.h"
//#include <Core/Application.h>
//#include "Core/Settings.h"
//#include "External/SOIL/SOIL.h"
//
//GLRenderContext::GLRenderContext(Platform* pPlatform) : RenderContext(pPlatform)
//{
//}
//
//void GLRenderContext::Initialize()
//{
//	RenderContext::Initialize();
//	// glClearColor(0.501960814f, 0.501960814f, 0.501960814f, 1.0f);
//}
//
//bool GLRenderContext::ResizeWorldBuffer()
//{
//	return false;
//}
//
//void GLRenderContext::DestroyTexture(const TextureReference* pTextureRef)
//{
//	// glDeleteTextures(1, ((GLuint*)pTextureRef->Descriptor));
//}
//
//TextureReadData* GLRenderContext::ReadTexture(const std::string& texturePath)
//{
//	TextureReadData* data = new TextureReadData();
//
//	/*	try to load the image	*/
//	int width, height, channels;
//	unsigned char* img = SOIL_load_image(texturePath.c_str(), &width, &height, &channels, 4);
//
//	data->m_Data = (uint32_t*)img;
//	data->m_Dimensions.x = width;
//	data->m_Dimensions.y = height;
//
//	return data;
//}
//
//void GLRenderContext::Deinitialize()
//{
//	// delete m_pVoxelPSO;
//}
//
//bool GLRenderContext::ModifyVoxel(uint32_t uiID, uint32_t uiColor, bool bOverwrite)
//{
//	return false;
//}
//
//uint32_t GLRenderContext::GetVoxel(uint32_t uiID) const
//{
//	return false;
//}
//
//const uint32_t* GLRenderContext::GetVoxelData() const
//{
//	return nullptr;
//}
//
//void GLRenderContext::ClearVoxels()
//{
//}
//
//void GLRenderContext::Clear()
//{
//	RenderContext::Clear();
//	// glClear(GL_COLOR_BUFFER_BIT);
//}
//
//void GLRenderContext::Present()
//{
//	// Set all render data to command list
//	for (auto& renderData : m_RenderList) {
//		if (renderData.m_type == VOXEL)
//			PresentVoxels(renderData);
//		else assert(false);
//	}
//
//	m_pPlatform->GetImguiSystem().Draw();
//}
//
//bool GLRenderContext::OnResize(uint32_t uiWidth, uint32_t uiHeight)
//{
//	if (!RenderContext::OnResize(uiWidth, uiHeight))
//		return false;
//
//	// Update textures
//	// glViewport(0, 0, static_cast<GLsizei>(uiWidth), static_cast<GLsizei>(uiHeight));
//
//	return true;
//}
//
//void GLRenderContext::LoadTexture(TextureReference* pTextureReference)
//{
//	if (!pTextureReference || pTextureReference->Texture)
//		return;
//
//	// std::string refPath = pTextureReference->GetRefPath();
//	// 
//	// GLuint result = SOIL_load_OGL_texture(
//	// 	pTextureReference->GetRefPath().c_str(),
//	// 	SOIL_LOAD_AUTO,
//	// 	SOIL_CREATE_NEW_ID,
//	// 	SOIL_FLAG_POWER_OF_TWO
//	// 	//| SOIL_FLAG_MIPMAPS
//	// 	//| SOIL_FLAG_MULTIPLY_ALPHA
//	// 	//| SOIL_FLAG_COMPRESS_TO_DXT
//	// 	| SOIL_FLAG_DDS_LOAD_DIRECT
//	// 	//| SOIL_FLAG_NTSC_SAFE_RGB
//	// 	//| SOIL_FLAG_CoCg_Y
//	// 	//| SOIL_FLAG_TEXTURE_RECTANGLE
//	// );
//	// 
//	// if (result < 1)
//	// 	m_pPlatform->GetApplication()->GetLoggingSystem().Log(LOGLEVEL_ERROR, "RenderContext", "Failed to load texture with path: " + refPath);
//	// else
//	// {
//	// 	if (!pTextureReference->Texture)
//	// 		pTextureReference->Texture = new GLuint();
//	// 
//	// 	if (!pTextureReference->Descriptor)
//	// 		pTextureReference->Descriptor = new GLuint();
//	// 
//	// 	*(GLuint*)pTextureReference->Texture = result;
//	// 	*(GLuint*)pTextureReference->Descriptor = result;
//	// }
//}
//
//void GLRenderContext::PresentVoxels(RenderData& renderData)
//{
//	m_bShouldUpdateVoxelWorld = false;
//}