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

#include "Core/Platform/Rendering/Vulkan/VKDevice.h"
#include "Core/Platform/Rendering/Vulkan/VKSwapchain.h"

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
	};

	Options ParseArgs(int argc, char** argv)
	{
		Options options;

		for (int i = 1; i < argc; ++i)
		{
			if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc)
				options.iMaxFrames = atoi(argv[++i]);
			else if (strcmp(argv[i], "--no-validation") == 0)
				options.bValidation = false;
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
		else if (!swapchain.Create(&device, surface, 1280, 720))
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

				if (options.iMaxFrames >= 0 && iFrame >= options.iMaxFrames)
					bRunning = false;
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
