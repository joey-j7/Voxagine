#pragma once

#include "../RenderBuffer.h"

#include "Core/Math.h"

#include "Core/ECS/Systems/Physics/VoxelGrid.h"

struct ConstantVoxelBuffer : public RenderBuffer
{
	struct Data {
		/* Camera model + view */
		Matrix4 Camera;

		/* Viewport width, height, projection value and aspect ratio */
		Vector4 ViewportValues;

		/* Light direction */
		Vector4 LightDirection;

		/* Selection grid line color */
		VColor SelectionColor = VColor(
			(unsigned char)255,
			(unsigned char)229,
			(unsigned char)59,
			(unsigned char)255
		);

		Vector3 GridPosition;
		Vector3 CameraPos;
		uint32_t SDFCount = 0;

		// Alignment
		uint32_t Padding1;
		uint32_t Padding2;
		uint32_t Padding3;
		uint32_t Padding4;
	};

	Data m_Data;

	virtual std::vector<uint32_t> GetStrides() const override {
		return {
			sizeof(Matrix4), sizeof(Vector4), sizeof(Vector4), sizeof(VColor), sizeof(Vector3), sizeof(Vector3), sizeof(uint32_t),
			// Alignment
			sizeof(uint32_t), sizeof(uint32_t), sizeof(uint32_t), sizeof(uint32_t)
		};
	};

	virtual uint32_t GetTotalSize() const override {
		return sizeof(Data);
	};

	static_assert((sizeof(Data) % 16) == 0, "CB size not padded correctly");
};

struct ConstantDepthBuffer : public RenderBuffer
{
	struct Data {
		/* Camera model + view + projection */
		Matrix4 CameraMVP;
		Vector4 CameraPos;
	};

	Data m_Data;

	virtual std::vector<uint32_t> GetStrides() const override {
		return {
			sizeof(Matrix4), sizeof(Vector4)
		};
	};

	virtual uint32_t GetTotalSize() const override {
		return sizeof(Data);
	};

	static_assert((sizeof(Data) % 16) == 0, "CB size not padded correctly");
};