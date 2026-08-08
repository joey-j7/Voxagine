/* Vulkan bring-up target.
 *
 * Opens an SDL3 window and drives VKDevice/VKSwapchain to a clear screen. This
 * is the first Linux milestone from the port plan, and it exercises the real
 * backend classes rather than a throwaway sample: the same VKDevice and
 * VKSwapchain will sit under VKRenderContext once the manager and pass layer
 * is ported.
 *
 * It deliberately does not link the engine. Core still needs RTTR (vendored as
 * headers only, no library) and FMOD before it can build on Linux, so this
 * target stays on the dependency-free side of that line and can be run today.
 *
 * Run headless with --frames N to make it usable from CI. */

#include "Core/Platform/Rendering/Vulkan/VKAllocator.h"
#include "Core/Platform/Rendering/Vulkan/VKCommandEngine.h"
#include "Core/Platform/Rendering/Vulkan/VKDescriptorLayout.h"
#include "Core/Platform/Rendering/Vulkan/VKDevice.h"
#include "Core/Platform/Rendering/Vulkan/VKSwapchain.h"
#include "Core/Platform/Rendering/Vulkan/VKUploadBuffer.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace
{
	struct Options
	{
		int iMaxFrames = -1;   /* -1 runs until the window is closed. */
		bool bValidation = true;
		bool bSelfTest = false;
	};

	/* Exercises VKAllocator and VKUploadBuffer against the real device.
	   Compiling proves nothing about whether the mapped pointer is writable or
	   the device address is meaningful. */
	bool RunSelfTest(VKDevice& device)
	{
		bool bPassed = true;

		VKAllocator allocator;
		allocator.Initialize(&device);

		VKUploadBuffer upload(&device, &allocator, 64 * 1024);

		/* A constant allocation must be aligned, mapped and addressable. */
		const VKUploadBuffer::Allocation first = upload.AllocateConstant(256);

		if (first.CPU == nullptr || first.GPU == 0 || first.Buffer == VK_NULL_HANDLE)
		{
			fprintf(stderr, "[selftest] constant allocation returned nothing\n");
			bPassed = false;
		}
		else
		{
			/* If the memory is not really host-visible this faults. */
			memset(first.CPU, 0xAB, 256);

			const uint8_t uiRead = static_cast<const uint8_t*>(first.CPU)[255];
			if (uiRead != 0xAB)
			{
				fprintf(stderr, "[selftest] readback mismatch: 0x%02X\n", uiRead);
				bPassed = false;
			}
		}

		/* A second allocation must not overlap the first. */
		const VKUploadBuffer::Allocation second = upload.AllocateConstant(256);

		if (second.CPU == first.CPU || second.GPU == first.GPU)
		{
			fprintf(stderr, "[selftest] second allocation aliased the first\n");
			bPassed = false;
		}

		/* Offsets must track the device address within the same page. */
		if (second.Buffer == first.Buffer &&
			second.GPU - first.GPU != second.Offset - first.Offset)
		{
			fprintf(stderr, "[selftest] device address and offset disagree\n");
			bPassed = false;
		}

		/* Oversized requests are rejected rather than throwing. */
		const VKUploadBuffer::Allocation tooBig = upload.Allocate(1024 * 1024, 256);

		if (tooBig.CPU != nullptr)
		{
			fprintf(stderr, "[selftest] oversized allocation was not rejected\n");
			bPassed = false;
		}

		/* Reset recycles pages, so the next allocation reuses the first slot. */
		upload.Reset();

		const VKUploadBuffer::Allocation recycled = upload.AllocateConstant(256);

		if (recycled.CPU != first.CPU)
		{
			fprintf(stderr, "[selftest] Reset did not recycle the page\n");
			bPassed = false;
		}

		printf("[selftest] upload buffer: %s\n", bPassed ? "pass" : "FAIL");

		/* Descriptor layouts. With validation enabled this is a real check of
		   the layout/pool/allocate path, which is where D3D12 root signatures
		   have the least in common with Vulkan. */
		bool bDescriptorsPassed = true;

		{
			VKDescriptorLayout layout;

			/* Mirrors what a pass declares: a constant buffer, a couple of
			   textures, a storage image and a sampler. The shifts keep b0 and
			   u0 from colliding the way they would in raw HLSL registers. */
			layout.AddConstantBuffer(0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
			layout.AddTexture(0, VK_SHADER_STAGE_FRAGMENT_BIT);
			layout.AddTexture(1, VK_SHADER_STAGE_FRAGMENT_BIT);
			layout.AddStorageImage(0, VK_SHADER_STAGE_COMPUTE_BIT);
			layout.AddSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT);

			if (!layout.Build(&device, VKSwapchain::m_uiFramesInFlight))
			{
				fprintf(stderr, "[selftest] descriptor layout build failed\n");
				bDescriptorsPassed = false;
			}
			else
			{
				for (uint32_t i = 0; i < VKSwapchain::m_uiFramesInFlight; ++i)
				{
					if (layout.Allocate() == VK_NULL_HANDLE)
					{
						fprintf(stderr, "[selftest] descriptor set %u failed to allocate\n", i);
						bDescriptorsPassed = false;
					}
				}

				/* The pool is sized for exactly that many; recycling must make
				   room again. */
				layout.ResetPool();

				if (layout.Allocate() == VK_NULL_HANDLE)
				{
					fprintf(stderr, "[selftest] allocation after ResetPool failed\n");
					bDescriptorsPassed = false;
				}
			}
		}

		/* Bindless is separate: it must be the highest binding, so building it
		   alongside a higher-numbered sampler would be rejected. */
		{
			VKDescriptorLayout bindless;
			bindless.AddConstantBuffer(0, VK_SHADER_STAGE_FRAGMENT_BIT);
			bindless.AddBindlessTextures(0, VK_SHADER_STAGE_FRAGMENT_BIT, 128);

			if (!bindless.Build(&device, 1) || bindless.Allocate() == VK_NULL_HANDLE)
			{
				fprintf(stderr, "[selftest] bindless layout failed\n");
				bDescriptorsPassed = false;
			}
		}

		printf("[selftest] descriptor layouts: %s\n", bDescriptorsPassed ? "pass" : "FAIL");

		/* Command engine. The point of interest is that Reset() waits on one
		   frame slot's timeline value rather than for the whole device - the
		   thing DXCommandEngine got wrong. */
		bool bEnginePassed = true;

		{
			CommandEngine::Info info;
			info.m_Type = CommandEngine::E_DIRECT;
			info.m_Name = "SelfTest";

			VKCommandEngine engine(&device, &allocator, info);

			if (!engine.Initialize())
			{
				fprintf(stderr, "[selftest] command engine init failed\n");
				bEnginePassed = false;
			}
			else
			{
				/* A resource whose transitions give the engine something real
				   to record. */
				VKResource image;
				if (!image.CreateImage(&device, &allocator, VK_IMAGE_TYPE_2D,
				                       VK_FORMAT_R8G8B8A8_UNORM, 64, 64, 1, 1,
				                       VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT))
				{
					fprintf(stderr, "[selftest] test image creation failed\n");
					bEnginePassed = false;
				}

				/* Submit several frames so the slots wrap and Reset() has a
				   prior submission to wait on. */
				for (uint32_t i = 0; i < VKCommandEngine::m_uiFrameCount * 3; ++i)
				{
					engine.Reset();
					engine.Start();

					engine.QueueBarrier(&image, (i % 2 == 0) ? E_STATE_COPY_DEST
					                                         : E_STATE_PIXEL_SHADER_RESOURCE);
					engine.ApplyBarriers();
					engine.Execute();
				}

				engine.WaitForGPU();

				/* One signal per Execute. */
				const uint64_t uiExpected = VKCommandEngine::m_uiFrameCount * 3;

				if (engine.GetValue() != uiExpected)
				{
					fprintf(stderr, "[selftest] fence value %llu, expected %llu\n",
					        static_cast<unsigned long long>(engine.GetValue()),
					        static_cast<unsigned long long>(uiExpected));
					bEnginePassed = false;
				}

				uint64_t uiCompleted = 0;
				vkGetSemaphoreCounterValue(device.Get(), engine.GetTimeline(), &uiCompleted);

				if (uiCompleted != uiExpected)
				{
					fprintf(stderr, "[selftest] timeline at %llu after WaitForGPU, expected %llu\n",
					        static_cast<unsigned long long>(uiCompleted),
					        static_cast<unsigned long long>(uiExpected));
					bEnginePassed = false;
				}

				/* The barriers must have left the resource in its last state. */
				if (image.GetState() != E_STATE_PIXEL_SHADER_RESOURCE)
				{
					fprintf(stderr, "[selftest] resource state not tracked through barriers\n");
					bEnginePassed = false;
				}

				image.Destroy();
			}
		}

		printf("[selftest] command engine: %s\n", bEnginePassed ? "pass" : "FAIL");

		return bPassed && bDescriptorsPassed && bEnginePassed;
	}

	Options ParseArgs(int argc, char** argv)
	{
		Options options;

		for (int i = 1; i < argc; ++i)
		{
			if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc)
				options.iMaxFrames = atoi(argv[++i]);
			else if (strcmp(argv[i], "--no-validation") == 0)
				options.bValidation = false;
			else if (strcmp(argv[i], "--selftest") == 0)
				options.bSelfTest = true;
		}

		return options;
	}
}

int main(int argc, char** argv)
{
	const Options options = ParseArgs(argc, argv);

	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		fprintf(stderr, "[sdl] SDL_Init failed: %s\n", SDL_GetError());
		return 1;
	}

	SDL_Window* pWindow = SDL_CreateWindow("Voxagine - Vulkan bring-up", 1280, 720,
	                                       SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

	if (pWindow == nullptr)
	{
		fprintf(stderr, "[sdl] SDL_CreateWindow failed: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	uint32_t uiExtensionCount = 0;
	const char* const* ppExtensions = SDL_Vulkan_GetInstanceExtensions(&uiExtensionCount);

	if (ppExtensions == nullptr)
	{
		fprintf(stderr, "[sdl] SDL_Vulkan_GetInstanceExtensions failed: %s\n", SDL_GetError());
		SDL_DestroyWindow(pWindow);
		SDL_Quit();
		return 1;
	}

	int iExit = 0;

	{
		VKDevice device;
		VKSwapchain swapchain;

		const std::vector<const char*> extensions(ppExtensions, ppExtensions + uiExtensionCount);

		VkSurfaceKHR surface = VK_NULL_HANDLE;

		if (!device.CreateInstance(extensions, options.bValidation))
		{
			iExit = 1;
		}
		else if (!SDL_Vulkan_CreateSurface(pWindow, device.GetInstance(), nullptr, &surface))
		{
			fprintf(stderr, "[sdl] SDL_Vulkan_CreateSurface failed: %s\n", SDL_GetError());
			iExit = 1;
		}
		else if (!device.CreateDevice(surface))
		{
			iExit = 1;
		}
		/* No vsync: bring-up runs a fixed number of frames and exits, so it
		   wants to finish rather than pace itself to the display. */
		else if (!swapchain.Create(&device, surface, 1280, 720, false))
		{
			iExit = 1;
		}
		else if (options.bSelfTest && !RunSelfTest(device))
		{
			iExit = 1;
		}
		else
		{
			printf("[bringup] device: %s\n", device.GetDeviceName().c_str());
			printf("[bringup] swapchain: %ux%u, %u images\n",
			       swapchain.GetExtent().width, swapchain.GetExtent().height,
			       swapchain.GetImageCount());

			bool bRunning = true;
			int iFrame = 0;

			while (bRunning)
			{
				/* Checked before presenting, so --frames 0 presents nothing. */
				if (options.iMaxFrames >= 0 && iFrame >= options.iMaxFrames)
					break;

				SDL_Event event;
				while (SDL_PollEvent(&event))
				{
					if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
						bRunning = false;
				}

				/* Cycle the clear colour so a stuck frame is obvious on screen. */
				const float fT = static_cast<float>(iFrame) * 0.01f;
				const float fColor[4] = {
					0.5f + 0.5f * sinf(fT),
					0.5f + 0.5f * sinf(fT + 2.094f),
					0.5f + 0.5f * sinf(fT + 4.189f),
					1.f
				};

				if (!swapchain.ClearAndPresent(fColor))
				{
					int iWidth = 0;
					int iHeight = 0;
					SDL_GetWindowSizeInPixels(pWindow, &iWidth, &iHeight);

					/* Minimised: nothing to present to, so idle instead of spinning. */
					if (iWidth == 0 || iHeight == 0)
					{
						SDL_Delay(16);
						continue;
					}

					if (!swapchain.Recreate(static_cast<uint32_t>(iWidth), static_cast<uint32_t>(iHeight)))
					{
						fprintf(stderr, "[bringup] swapchain recreation failed\n");
						iExit = 1;
						break;
					}
				}

				++iFrame;
			}

			printf("[bringup] presented %d frames\n", iFrame);
		}

		/* Order matters: swapchain objects belong to the device. */
		swapchain.Destroy();

		if (surface != VK_NULL_HANDLE)
			SDL_Vulkan_DestroySurface(device.GetInstance(), surface, nullptr);

		device.Destroy();
	}

	SDL_DestroyWindow(pWindow);
	SDL_Quit();

	return iExit;
}
