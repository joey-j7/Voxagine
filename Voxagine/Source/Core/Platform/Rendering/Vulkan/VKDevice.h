#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

/* Instance, physical device, logical device and queues.
 *
 * Deliberately free of engine dependencies: it takes a surface and hands back
 * Vulkan objects, so it can be brought up and tested before the rest of the
 * renderer is ported. VKRenderContext will own one of these. */
class VKDevice
{
public:
	VKDevice() = default;
	~VKDevice();

	VKDevice(const VKDevice&) = delete;
	VKDevice& operator=(const VKDevice&) = delete;

	/* a_InstanceExtensions comes from the windowing layer (SDL_Vulkan_GetInstanceExtensions). */
	bool CreateInstance(const std::vector<const char*>& a_InstanceExtensions, bool bEnableValidation);

	/* Must be called after the window layer has produced a surface from GetInstance(). */
	bool CreateDevice(VkSurfaceKHR surface);

	void Destroy();

	VkInstance GetInstance() const { return m_Instance; }
	VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
	VkDevice Get() const { return m_Device; }

	VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
	VkQueue GetPresentQueue() const { return m_PresentQueue; }

	/* vkQueueSubmit and vkQueuePresentKHR require the queue to be externally
	   synchronized, and every engine here shares one graphics queue. Resource
	   loading submits uploads from job threads while the main thread submits
	   the frame, so this is not theoretical - the validation layers catch it
	   as a threading error. Hold this across any queue operation. */
	std::mutex& GetQueueMutex() const { return m_QueueMutex; }

	uint32_t GetGraphicsQueueFamily() const { return m_uiGraphicsFamily; }
	uint32_t GetPresentQueueFamily() const { return m_uiPresentFamily; }

	const std::string& GetDeviceName() const { return m_DeviceName; }

	/* RENDERING_PLAN.md Phase 0: whether vkCmdWriteTimestamp2 is usable on
	   this device's graphics/compute queues, and the nanoseconds-per-tick
	   needed to turn a timestamp delta into milliseconds. */
	bool SupportsTimestamps() const { return m_bTimestampsSupported; }
	float GetTimestampPeriod() const { return m_fTimestampPeriod; }

private:
	bool PickPhysicalDevice(VkSurfaceKHR surface);
	bool FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface,
	                       uint32_t& uiGraphics, uint32_t& uiPresent) const;
	bool HasSwapchainSupport(VkPhysicalDevice device) const;

	mutable std::mutex m_QueueMutex;

	VkInstance m_Instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_Device = VK_NULL_HANDLE;

	VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
	VkQueue m_PresentQueue = VK_NULL_HANDLE;

	uint32_t m_uiGraphicsFamily = UINT32_MAX;
	uint32_t m_uiPresentFamily = UINT32_MAX;

	std::string m_DeviceName;
	bool m_bValidationEnabled = false;

	bool m_bTimestampsSupported = false;
	float m_fTimestampPeriod = 0.0f;
};
