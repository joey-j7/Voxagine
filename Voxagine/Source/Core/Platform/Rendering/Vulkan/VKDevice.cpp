#include "VKDevice.h"

#include <cstdio>
#include <cstring>
#include <set>

namespace
{
	const char* const kValidationLayer = "VK_LAYER_KHRONOS_validation";
	const char* const kDeviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT,
		const VkDebugUtilsMessengerCallbackDataEXT* pData,
		void*)
	{
		if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
			fprintf(stderr, "[vulkan] %s\n", pData->pMessage);

		return VK_FALSE;
	}

	bool HasLayer(const char* pName)
	{
		uint32_t uiCount = 0;
		vkEnumerateInstanceLayerProperties(&uiCount, nullptr);

		std::vector<VkLayerProperties> layers(uiCount);
		vkEnumerateInstanceLayerProperties(&uiCount, layers.data());

		for (const VkLayerProperties& layer : layers)
		{
			if (strcmp(layer.layerName, pName) == 0)
				return true;
		}

		return false;
	}
}

VKDevice::~VKDevice()
{
	Destroy();
}

bool VKDevice::CreateInstance(const std::vector<const char*>& a_InstanceExtensions, bool bEnableValidation)
{
	m_bValidationEnabled = bEnableValidation && HasLayer(kValidationLayer);

	if (bEnableValidation && !m_bValidationEnabled)
		printf("[vulkan] validation layer requested but not installed, continuing without it\n");

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Voxagine";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Voxagine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	std::vector<const char*> extensions = a_InstanceExtensions;
	if (m_bValidationEnabled)
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (m_bValidationEnabled)
	{
		createInfo.enabledLayerCount = 1;
		createInfo.ppEnabledLayerNames = &kValidationLayer;
	}

	if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS)
	{
		fprintf(stderr, "[vulkan] vkCreateInstance failed\n");
		return false;
	}

	if (m_bValidationEnabled)
	{
		VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
		debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		                        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugInfo.pfnUserCallback = DebugCallback;

		PFN_vkCreateDebugUtilsMessengerEXT pCreate =
			reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
				vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT"));

		if (pCreate != nullptr)
			pCreate(m_Instance, &debugInfo, nullptr, &m_DebugMessenger);
	}

	return true;
}

bool VKDevice::HasSwapchainSupport(VkPhysicalDevice device) const
{
	uint32_t uiCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &uiCount, nullptr);

	std::vector<VkExtensionProperties> available(uiCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &uiCount, available.data());

	for (const VkExtensionProperties& ext : available)
	{
		if (strcmp(ext.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
			return true;
	}

	return false;
}

bool VKDevice::FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface,
                                 uint32_t& uiGraphics, uint32_t& uiPresent) const
{
	uint32_t uiCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &uiCount, nullptr);

	std::vector<VkQueueFamilyProperties> families(uiCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &uiCount, families.data());

	uiGraphics = UINT32_MAX;
	uiPresent = UINT32_MAX;

	for (uint32_t i = 0; i < uiCount; ++i)
	{
		if (families[i].queueCount == 0)
			continue;

		if (uiGraphics == UINT32_MAX && (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
			uiGraphics = i;

		VkBool32 bPresent = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &bPresent);

		if (uiPresent == UINT32_MAX && bPresent == VK_TRUE)
			uiPresent = i;
	}

	return uiGraphics != UINT32_MAX && uiPresent != UINT32_MAX;
}

bool VKDevice::PickPhysicalDevice(VkSurfaceKHR surface)
{
	uint32_t uiCount = 0;
	vkEnumeratePhysicalDevices(m_Instance, &uiCount, nullptr);

	if (uiCount == 0)
	{
		fprintf(stderr, "[vulkan] no physical devices\n");
		return false;
	}

	std::vector<VkPhysicalDevice> devices(uiCount);
	vkEnumeratePhysicalDevices(m_Instance, &uiCount, devices.data());

	VkPhysicalDevice fallback = VK_NULL_HANDLE;
	uint32_t uiFallbackGraphics = UINT32_MAX;
	uint32_t uiFallbackPresent = UINT32_MAX;

	for (VkPhysicalDevice device : devices)
	{
		uint32_t uiGraphics = 0;
		uint32_t uiPresent = 0;

		if (!HasSwapchainSupport(device))
			continue;

		if (!FindQueueFamilies(device, surface, uiGraphics, uiPresent))
			continue;

		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(device, &props);

		/* Prefer a discrete GPU, but take whatever presents if there is none. */
		if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			m_PhysicalDevice = device;
			m_uiGraphicsFamily = uiGraphics;
			m_uiPresentFamily = uiPresent;
			m_DeviceName = props.deviceName;
			return true;
		}

		if (fallback == VK_NULL_HANDLE)
		{
			fallback = device;
			uiFallbackGraphics = uiGraphics;
			uiFallbackPresent = uiPresent;
			m_DeviceName = props.deviceName;
		}
	}

	if (fallback == VK_NULL_HANDLE)
	{
		fprintf(stderr, "[vulkan] no device can present to this surface\n");
		return false;
	}

	m_PhysicalDevice = fallback;
	m_uiGraphicsFamily = uiFallbackGraphics;
	m_uiPresentFamily = uiFallbackPresent;

	return true;
}

bool VKDevice::CreateDevice(VkSurfaceKHR surface)
{
	if (!PickPhysicalDevice(surface))
		return false;

	const float fPriority = 1.f;

	/* Graphics and present are usually the same family; only ask once. */
	std::set<uint32_t> uniqueFamilies = { m_uiGraphicsFamily, m_uiPresentFamily };
	std::vector<VkDeviceQueueCreateInfo> queueInfos;

	for (uint32_t uiFamily : uniqueFamilies)
	{
		VkDeviceQueueCreateInfo queueInfo{};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = uiFamily;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &fPriority;

		queueInfos.push_back(queueInfo);
	}

	/* Synchronization2 gives us the split access/stage masks that VKTranslate.h
	   expands engine resource states into. */
	VkPhysicalDeviceSynchronization2Features sync2{};
	sync2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
	sync2.synchronization2 = VK_TRUE;

	/* CommandEngine exposes a monotonically increasing fence value that other
	   engines wait on - an ID3D12Fence. Timeline semaphores are the direct
	   equivalent. */
	VkPhysicalDeviceTimelineSemaphoreFeatures timeline{};
	timeline.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
	timeline.timelineSemaphore = VK_TRUE;
	timeline.pNext = &sync2;

	/* Buffer::GetGPUAddress() returns a uint64 that used to be a
	   D3D12_GPU_VIRTUAL_ADDRESS. bufferDeviceAddress is the equivalent and
	   keeps every call site working unchanged. */
	VkPhysicalDeviceBufferDeviceAddressFeatures bufferAddress{};
	bufferAddress.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
	bufferAddress.bufferDeviceAddress = VK_TRUE;
	bufferAddress.pNext = &timeline;

	/* Bindless arrays: RenderPass::Data::m_uiBindlessResourceCount declares a
	   variable-sized texture array, which needs descriptor indexing. */
	VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexing{};
	descriptorIndexing.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
	descriptorIndexing.descriptorBindingVariableDescriptorCount = VK_TRUE;
	descriptorIndexing.descriptorBindingPartiallyBound = VK_TRUE;
	descriptorIndexing.runtimeDescriptorArray = VK_TRUE;
	descriptorIndexing.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	descriptorIndexing.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
	descriptorIndexing.pNext = &bufferAddress;

	VkPhysicalDeviceDynamicRenderingFeatures dynamicRendering{};
	dynamicRendering.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
	dynamicRendering.dynamicRendering = VK_TRUE;
	dynamicRendering.pNext = &descriptorIndexing;

	VkDeviceCreateInfo deviceInfo{};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.pNext = &dynamicRendering;
	deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
	deviceInfo.pQueueCreateInfos = queueInfos.data();
	deviceInfo.enabledExtensionCount = 1;
	deviceInfo.ppEnabledExtensionNames = kDeviceExtensions;

	if (vkCreateDevice(m_PhysicalDevice, &deviceInfo, nullptr, &m_Device) != VK_SUCCESS)
	{
		fprintf(stderr, "[vulkan] vkCreateDevice failed\n");
		return false;
	}

	vkGetDeviceQueue(m_Device, m_uiGraphicsFamily, 0, &m_GraphicsQueue);
	vkGetDeviceQueue(m_Device, m_uiPresentFamily, 0, &m_PresentQueue);

	return true;
}

void VKDevice::Destroy()
{
	if (m_Device != VK_NULL_HANDLE)
	{
		vkDestroyDevice(m_Device, nullptr);
		m_Device = VK_NULL_HANDLE;
	}

	if (m_DebugMessenger != VK_NULL_HANDLE)
	{
		PFN_vkDestroyDebugUtilsMessengerEXT pDestroy =
			reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
				vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT"));

		if (pDestroy != nullptr)
			pDestroy(m_Instance, m_DebugMessenger, nullptr);

		m_DebugMessenger = VK_NULL_HANDLE;
	}

	if (m_Instance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(m_Instance, nullptr);
		m_Instance = VK_NULL_HANDLE;
	}
}
