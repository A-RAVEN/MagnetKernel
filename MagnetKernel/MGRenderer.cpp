#include "MGMath.h"
#include "MGDebug.h"
#include "MGRenderer.h"
#include "MGImageFunctions.h"




MGRenderer::MGRenderer()
{
}

MGRenderer::MGRenderer(MGInstance* instance, MGWindow& window)
{
	VkSurfaceKHR surface;
	window.getWindowSurface(&surface);
	_selectPhysicalDevice(instance, surface);
	_initLogicalDevice();
	_initCommandPools();
	SwapChain = new MGSwapChain(this, &window);

	//...
}

MGRenderer::~MGRenderer()
{
}

void MGRenderer::releaseRenderer()
{
	//...

	SwapChain->releaseSwapChain();
	delete SwapChain;
	_deInitCommandPools();
	vkDestroyDevice(LogicalDevice, nullptr);
}

void MGRenderer::_selectPhysicalDevice(MGInstance* instance, VkSurfaceKHR windowSurface)
{
	OutputSurface = windowSurface;
	int filter = VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT;
	std::vector<MGDevice> devices = instance->filterDevices(filter);
	if (devices.size() > 0) {
		bool selected = false;
		for (auto device : devices)
		{
			for (int index = 0; index < device.queueFamilies.size(); index++)
			{
				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(device.device, index, windowSurface, &presentSupport);
				if (device.queueFamilies[index].queueFlags & filter == filter && presentSupport)
				{
					selected = true;
					PhysicalDevice = device;
					GraphicQueueFamilyIndex = TransferQueueFamilyIndex = ComputeQueueFamilyIndex = PresentQueueFamilyIndex = index;
					break;
				}
			}
			if (selected)
				break;
		}
		if (!selected)
		{
			for (auto device : devices)
			{
				bool canpresent = false;
				for (int index = 0; index < device.queueFamilies.size(); index++)
				{
					VkBool32 presentSupport = false;
					vkGetPhysicalDeviceSurfaceSupportKHR(device.device, index, windowSurface, &presentSupport);
					if (presentSupport)
					{
						selected = true;
						canpresent = true;
						PresentQueueFamilyIndex = index;
						PhysicalDevice = device;
					}
					if (device.queueFamilies[index].queueFlags & VK_QUEUE_GRAPHICS_BIT)
					{
						GraphicQueueFamilyIndex = index;
					}
					if (device.queueFamilies[index].queueFlags & VK_QUEUE_TRANSFER_BIT)
					{
						TransferQueueFamilyIndex = index;
					}
					if (device.queueFamilies[index].queueFlags & VK_QUEUE_COMPUTE_BIT)
					{
						ComputeQueueFamilyIndex = index;
					}
				}
				if (canpresent)
					break;
			}
		}
		if (!selected)
		{
			//Error report
		}
	}
	else
	{
		//No Device report
	}
}



void MGRenderer::_initLogicalDevice()
{
	std::vector<int> queuefamilies = { GraphicQueueFamilyIndex ,TransferQueueFamilyIndex ,ComputeQueueFamilyIndex ,PresentQueueFamilyIndex };
	std::vector<int> uniqueQueueFamilies = mgm::calculateNoneRepeatingArray(queuefamilies);

	float queuePriority = 1.0f;

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	for (int queueFamilyIndex : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
		queueCreateInfo.queueCount = PhysicalDevice.queueFamilies[queueFamilyIndex].queueCount;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
#if BUILD_ENABLE_DEBUG
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_LUNARG_standard_validation", "VK_LAYER_LUNARG_core_validation"
	};
	MGCheckValidationLayerSupport(validationLayers, EXTENSION_TYPE_DEVICE, PhysicalDevice.device);
	createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	createInfo.ppEnabledLayerNames = validationLayers.data();
#else
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = VK_NULL_HANDLE;
#endif //ERR_CHECK

	MGCheckVKResultERR(vkCreateDevice(PhysicalDevice.device, &createInfo, nullptr, &LogicalDevice),"Failed to create Logical Device");

//获取不同功能的Queue
	for (int queueFamilyIndex : uniqueQueueFamilies) 
	{
		for (int queueIndex = 0; queueIndex < PhysicalDevice.queueFamilies[queueFamilyIndex].queueCount; queueIndex++) {
			VkQueue queue;
			vkGetDeviceQueue(LogicalDevice, queueFamilyIndex, queueIndex, &queue);
			ActiveQueues.push_back(queue);
			int lastIndex = ActiveQueues.size() - 1;
			if (queueFamilyIndex == GraphicQueueFamilyIndex)
			{
				GraphicQueuesIndices.push_back(lastIndex);
			}
			if (queueFamilyIndex == TransferQueueFamilyIndex)
			{
				TransferQueuesIndices.push_back(lastIndex);
			}
			if (queueFamilyIndex == ComputeQueueFamilyIndex)
			{
				ComputeQueuesIndices.push_back(lastIndex);
			}
			if (queueFamilyIndex == PresentQueueFamilyIndex)
			{
				PresentQueuesIndices.push_back(lastIndex);
			}
		}

	}
}

void MGRenderer::_initCommandPools()
{
	std::vector<int> queuefamilies = { GraphicQueueFamilyIndex ,TransferQueueFamilyIndex ,ComputeQueueFamilyIndex ,PresentQueueFamilyIndex };
	std::vector<int> uniqueQueueFamilies = mgm::calculateNoneRepeatingArray(queuefamilies);

	for (auto Index : uniqueQueueFamilies) {
		VkCommandPool pool;

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = Index;
		poolInfo.flags = 0; //
		vkCreateCommandPool(LogicalDevice, &poolInfo, nullptr, &pool);

		CommandPools.push_back(pool);
		int lastInex = CommandPools.size() - 1;

		if (Index == GraphicQueueFamilyIndex)
		{
			GraphicCommandPoolIndex = Index;
		}
		if (Index == TransferQueueFamilyIndex)
		{
			TransferCommandPoolIndex = Index;
		}
		if (Index == ComputeQueueFamilyIndex)
		{
			ComputeCommandPoolIndex = Index;
		}
		if (Index == PresentQueueFamilyIndex)
		{
			PresentCommandPoolIndex = Index;
		}
	}
}

void MGRenderer::_deInitCommandPools()
{
	for (auto commandPool : CommandPools) {
		vkDestroyCommandPool(LogicalDevice, commandPool, nullptr);
	}
	CommandPools.clear();
}

MGSwapChain::MGSwapChain(MGRenderer* renderer,MGWindow* window)
{
	OwningRenderer = renderer;
	RelatingWindow = window;
	VkSurfaceKHR surface;
	RelatingWindow->getWindowSurface(&surface);

	MGSurfaceImageSupportDetails swapChainSupport = RelatingWindow->querySurfaceImageSupport(OwningRenderer->PhysicalDevice.device);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, RelatingWindow->Width, RelatingWindow->Hight);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	//QueueFamilyIndices indices = selectQueueFamily(physicalDevice);
	std::vector<uint32_t> RawQueueFamilyIndices = { (uint32_t)OwningRenderer->GraphicQueueFamilyIndex, (uint32_t)OwningRenderer->PresentQueueFamilyIndex };
	std::vector<uint32_t> QueueFamilyIndices = mgm::calculateNoneRepeatingArray(RawQueueFamilyIndices);

	if (QueueFamilyIndices.size() > 1) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = QueueFamilyIndices.size();
		createInfo.pQueueFamilyIndices = QueueFamilyIndices.data();
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}
	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(OwningRenderer->LogicalDevice, &createInfo, nullptr, &swapchain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	SwapchainImageFormat = surfaceFormat.format;
	SwapchainExtent = extent;

	vkGetSwapchainImagesKHR(OwningRenderer->LogicalDevice, swapchain, &imageCount, nullptr);
	SwapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(OwningRenderer->LogicalDevice, swapchain, &imageCount, SwapchainImages.data());

	SwapchainImageViews.resize(SwapchainImages.size());

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.layerCount = 1;

	for (uint32_t i = 0; i < SwapchainImages.size(); i++) {
		SwapchainImageViews[i] = mgCreateImageView2D(OwningRenderer->LogicalDevice, SwapchainImages[i], SwapchainImageFormat, subresourceRange);
	}
}

MGSwapChain::~MGSwapChain()
{

}

void MGSwapChain::releaseSwapChain()
{
	for (size_t i = 0; i < SwapchainImageViews.size(); i++) {
		vkDestroyImageView(OwningRenderer->LogicalDevice, SwapchainImageViews[i], nullptr);
	}
	vkDestroySwapchainKHR(OwningRenderer->LogicalDevice, swapchain, nullptr);
}

VkSurfaceFormatKHR MGSwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	//如果支持任意格式，直接返回理想格式
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}
	//如果仅支持部分格式，尝试找到理想的格式
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	//如果不支持理想的格式，直接返回第种格式
	return availableFormats[0];
}

VkPresentModeKHR MGSwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
{
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			bestMode = availablePresentMode;
		}
	}

	return bestMode;
}

VkExtent2D MGSwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities,int actualWidth,int actualHeight)
{
	if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)()) {
		return capabilities.currentExtent;
	}
	else {
		VkExtent2D actualExtent = { (uint32_t)actualWidth, (uint32_t)actualHeight };

		actualExtent.width = std::max<uint32_t>(capabilities.minImageExtent.width, std::min<uint32_t>(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max<uint32_t>(capabilities.minImageExtent.height, std::min<uint32_t>(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}
