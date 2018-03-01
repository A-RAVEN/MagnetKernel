#include "MGMath.h"
#include "MGDebug.h"
#include "MGRenderer.h"
//#include "MGImageLoad.h"
#include "MGShare.h"
#include "MGPipelineManager.h"

const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0,
	4, 5, 6, 6, 7, 4
};

MGRenderer::MGRenderer()
{
	UNIFORM_BIND_POINT_CAMERA = 0;
	UNIFORM_BIND_POINT_MODEL_MATRIX = UNIFORM_BIND_POINT_CAMERA + sizeof(UniformBufferObject);
	UNIFORM_BIND_POINT_LIGHT = UNIFORM_BIND_POINT_MODEL_MATRIX + sizeof(mgm::mat4);
}

MGRenderer::MGRenderer(MGInstance* instance, MGWindow& window)
{
	UNIFORM_BIND_POINT_CAMERA = 0;
	UNIFORM_BIND_POINT_MODEL_MATRIX = UNIFORM_BIND_POINT_CAMERA + sizeof(UniformBufferObject);
	UNIFORM_BIND_POINT_LIGHT = UNIFORM_BIND_POINT_MODEL_MATRIX + sizeof(mgm::mat4);
	VkSurfaceKHR surface;
	window.getWindowSurface(&surface);
	_selectPhysicalDevice(instance, surface);
	_initLogicalDevice();
	_initCommandPools();
	SwapChain = new MGSwapChain(this, &window);
	_initSamplers();
	_initSemaphores();

	Pipeline = new MGPipelineManager(this);
	Pipeline->initPipeline();


	SwapChain->createSwapchainFramebuffers(Pipeline->renderPass, true);

	_initPrimaryCommandBuffer();

	window.RelatingRenderer = this;
	//...
}

MGRenderer::~MGRenderer()
{
}

void MGRenderer::releaseRenderer()
{
	//...
	vkQueueWaitIdle(getQueue(MG_USE_GRAPHIC,0));
	Pipeline->releasePipeline();
	delete Pipeline;
	_deInitSemaphores();
	_deInitSamplers();
	_deinitRenderFences();
	SwapChain->releaseSwapChain();
	delete SwapChain;
	_deInitCommandPools();
	vkDestroyDevice(LogicalDevice, nullptr);
}

void MGRenderer::updateUniforms()
{
	//Pipeline->updatePipeline();
}

void MGRenderer::renderFrame()
{
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(LogicalDevice, SwapChain->Swapchain, (std::numeric_limits<uint64_t>::max)(), RendererSemaphores.MG_SEMAPHORE_SWAPCHAIN_IMAGE_AVAILABLE, VK_NULL_HANDLE, &imageIndex);
	
	_recordPrimaryCommandBuffer(imageIndex);

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &PrimaryCommandBuffers[imageIndex];
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &RendererSemaphores.MG_SEMAPHORE_SWAPCHAIN_IMAGE_AVAILABLE;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &RendererSemaphores.MG_SEMAPHORE_RENDER_FINISH;
	vkResetFences(LogicalDevice, 1, &PrimaryCommandBufferFences[imageIndex]);
	MGCheckVKResultERR(vkQueueSubmit(getQueue(MG_USE_GRAPHIC,0), 1, &submitInfo, PrimaryCommandBufferFences[imageIndex]), "Render command buffer submit failed!");

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &RendererSemaphores.MG_SEMAPHORE_RENDER_FINISH;
	VkSwapchainKHR swapChains[] = { SwapChain->Swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional


	result = vkQueuePresentKHR(getQueue(MG_USE_PRESENT, 0), &presentInfo);
}

void MGRenderer::OnWindowResized()
{
	vkQueueWaitIdle(getQueue(MG_USE_GRAPHIC, 0));
	SwapChain->releaseSwapChain();
	SwapChain->initSwapChain();
	SwapChain->createSwapchainFramebuffers(Pipeline->renderPass, true);
	//_recordPrimaryCommandBuffers();
}

uint32_t MGRenderer::mgFindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(PhysicalDevice.device, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

void MGRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(getCommandPool(MG_USE_GRAPHIC));

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0; // Optional
	copyRegion.dstOffset = 0; // Optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(commandBuffer, getQueue(MG_USE_GRAPHIC, 1), getCommandPool(MG_USE_GRAPHIC));
}

void MGRenderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(getCommandPool(MG_USE_TRANSFER));
	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};
	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);
	endSingleTimeCommands(commandBuffer, getQueue(MG_USE_TRANSFER, 1), getCommandPool(MG_USE_TRANSFER));
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
				if ((device.queueFamilies[index].queueFlags & filter) == filter && presentSupport)
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

		std::vector<float>* queuePriorities = new std::vector<float>(PhysicalDevice.queueFamilies[queueFamilyIndex].queueCount,1.0f);

		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
		queueCreateInfo.queueCount = PhysicalDevice.queueFamilies[queueFamilyIndex].queueCount;
		//queueCreateInfo.queueCount = 2;
		queueCreateInfo.pQueuePriorities = queuePriorities->data();
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
#ifdef _DEBUG
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
		for (uint32_t queueIndex = 0; queueIndex < PhysicalDevice.queueFamilies[queueFamilyIndex].queueCount; queueIndex++) {
		//for (int queueIndex = 0; queueIndex < 1; queueIndex++) {
			VkQueue queue;
			vkGetDeviceQueue(LogicalDevice, queueFamilyIndex, queueIndex, &queue);
			ActiveQueues.push_back(queue);
			uint32_t lastIndex = ActiveQueues.size() - 1;
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
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; //
		vkCreateCommandPool(LogicalDevice, &poolInfo, nullptr, &pool);

		CommandPools.push_back(pool);
		uint32_t lastInex = CommandPools.size() - 1;

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

void MGRenderer::_initPrimaryCommandBuffer()
{
	uint32_t size = static_cast<uint32_t>(SwapChain->getSwapchainImageSize());
	PrimaryCommandBuffers.resize(size);

	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = 0;
	for (uint32_t i = 0; i < size; ++i)
	{
		VkFence renderFence;
		vkCreateFence(LogicalDevice, &fenceCreateInfo, NULL, &renderFence);
		PrimaryCommandBufferFences.push_back(renderFence);
		PrimaryFenceInited.push_back(false);
	}
	vkResetFences(LogicalDevice, size, PrimaryCommandBufferFences.data());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = getCommandPool(MG_USE_GRAPHIC);
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)size;
	if (vkAllocateCommandBuffers(LogicalDevice, &allocInfo, PrimaryCommandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
	//_recordPrimaryCommandBuffers();
}

void MGRenderer::_deinitRenderFences()
{
	for (std::vector<VkFence>::const_iterator it = PrimaryCommandBufferFences.begin(); it != PrimaryCommandBufferFences.end(); ++it)
	{
		vkDestroyFence(LogicalDevice, *it, nullptr);
	}
}

void MGRenderer::_recordPrimaryCommandBuffers()
{
	for (int i = 0; i < PrimaryCommandBuffers.size(); i++) {
		_recordPrimaryCommandBuffer(i);
	}
}

void MGRenderer::_recordPrimaryCommandBuffer(uint8_t index)
{
	if (PrimaryFenceInited[index])
	{
		MGCheckVKResultERR(vkWaitForFences(LogicalDevice, 1, &PrimaryCommandBufferFences[index], VK_TRUE, 100000000), "Waiting Render Fence Problem !");
		//vkResetFences(LogicalDevice, 1, &PrimaryCommandBufferFences[index]);
	}
	else
	{
		PrimaryFenceInited[index] = true;
	}

	VkCommandBuffer PrimaryCommandBuffer = PrimaryCommandBuffers[index];
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(PrimaryCommandBuffer, &beginInfo);

	Pipeline->cmdExecute(PrimaryCommandBuffer, index);

	vkEndCommandBuffer(PrimaryCommandBuffer);
}

void MGRenderer::_initSamplers()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	samplerInfo.anisotropyEnable = VK_TRUE;//当实际像素数量小于图片像素数量时，使用anistropic采样可以避免模糊
	samplerInfo.maxAnisotropy = 16;//采样点

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;//超出范围的颜色

	samplerInfo.unnormalizedCoordinates = VK_FALSE;//为真时，映射坐标范围为[0,图片长宽]，为假时，映射坐标范围为[0,1]

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;
	
	MGCheckVKResultERR(vkCreateSampler(LogicalDevice, &samplerInfo, nullptr, &TextureSamplers.MG_SAMPLER_NORMAL),"Failed to create GM_SAMPLER_NORMAL");
}

void MGRenderer::_deInitSamplers()
{
	vkDestroySampler(LogicalDevice, TextureSamplers.MG_SAMPLER_NORMAL, nullptr);
}

void MGRenderer::_initSemaphores()
{
	RendererSemaphores.initSemaphores(LogicalDevice);
}

void MGRenderer::_deInitSemaphores()
{
	RendererSemaphores.deinitSemaphores(LogicalDevice);
}

VkCommandBuffer MGRenderer::beginSingleTimeCommands(VkCommandPool cmdPool)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = cmdPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(LogicalDevice, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void MGRenderer::endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue submitqueue, VkCommandPool cmdPool, MGArrayStruct<VkSemaphore> waitSemaphores, MGArrayStruct<VkSemaphore> signalSemaphores)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	MGCheckVKResultERR(vkQueueSubmit(submitqueue, 1, &submitInfo, VK_NULL_HANDLE),"Quick command buffer submit failed!");
	vkQueueWaitIdle(submitqueue);

	vkFreeCommandBuffers(LogicalDevice, cmdPool, 1, &commandBuffer);
}

void MGRenderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage & img, VkDeviceMemory & imageMemory)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;//第一个transform不会将image的内容丢弃，而VK_IMAGE_LAYOUT_UNDEFINED可能会将内容丢弃，适用于attachementimage
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0; // Optional，用于稀疏纹理，如体素纹理并不是所有像素都要存储内容，无需给所有像素分配空间！！！！！！！！！！！！！！！！
	if (vkCreateImage(LogicalDevice, &imageInfo, nullptr, &img) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(LogicalDevice, img, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = mgFindMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(LogicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(LogicalDevice, img, imageMemory, 0);
}

void MGRenderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer & buffer, VkDeviceMemory & bufferMemory)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(LogicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(LogicalDevice, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = mgFindMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(LogicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(LogicalDevice, buffer, bufferMemory, 0);
}

void MGRenderer::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(getCommandPool(MG_USE_GRAPHIC));
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,//barrier在此阶段开始之后执行
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,//barrier在此阶段结束之前完成
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
	endSingleTimeCommands(commandBuffer, getQueue(MG_USE_TRANSFER, 2), getCommandPool(MG_USE_TRANSFER));
}

VkImageView MGRenderer::createImageView2D(VkImage image, VkFormat format, VkImageSubresourceRange subresourceRange)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange = subresourceRange;

	VkImageView imageView;
	MGCheckVKResultERR(vkCreateImageView(LogicalDevice, &viewInfo, nullptr, &imageView), "Failed to create ImageView");

	return imageView;
}

VkFormat MGRenderer::findSupportedImageFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(PhysicalDevice.device, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}
	MGThrowError(true, "Failed to find supported image format !");
}

VkCommandPool MGRenderer::getCommandPool(MGUses use)
{
	switch (use)
	{
	case MG_USE_GRAPHIC:
		return CommandPools[GraphicCommandPoolIndex];
		break;
	case MG_USE_TRANSFER:
		return CommandPools[TransferCommandPoolIndex];
		break;
	case MG_USE_COMPUTE:
		return CommandPools[ComputeCommandPoolIndex];
		break;
	case MG_USE_PRESENT:
		return CommandPools[PresentCommandPoolIndex];
		break;
	default:
		MGThrowError(true, "Cannot get command pool");
		break;
	}
	
}

VkQueue MGRenderer::getQueue(MGUses use, int idealID)
{
	switch (use)
	{
	case MG_USE_GRAPHIC:
		return ActiveQueues[GraphicQueuesIndices[std::min<uint32_t>(GraphicQueuesIndices.size() - 1, idealID)]];
		break;
	case MG_USE_TRANSFER:
		return ActiveQueues[TransferQueuesIndices[std::min<uint32_t>(TransferQueuesIndices.size() - 1, idealID)]];
		break;
	case MG_USE_COMPUTE:
		return ActiveQueues[ComputeQueuesIndices[std::min<uint32_t>(ComputeQueuesIndices.size() - 1, idealID)]];
		break;
	case MG_USE_PRESENT:
		return ActiveQueues[PresentQueuesIndices[std::min<uint32_t>(PresentQueuesIndices.size() - 1, idealID)]];
		break;
	default:
		MGThrowError(true, "Cannot get ideal queue");
		break;
	}
}

VkViewport MGRenderer::createFullScreenViewport()
{
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)SwapChain->SwapchainExtent.width;
	viewport.height = (float)SwapChain->SwapchainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	return viewport;
}

VkRect2D MGRenderer::createFullScreenRect()
{
	VkRect2D rect = {};
	rect.offset = { 0, 0 };
	rect.extent = SwapChain->SwapchainExtent;
	return rect;
}

VkFence MGRenderer::getPrimaryCmdBufferFence(int ID)
{
	return PrimaryCommandBufferFences[ID];
}

MGSwapChain::MGSwapChain(MGRenderer* renderer,MGWindow* window)
{
	OwningRenderer = renderer;
	RelatingWindow = window;
	initSwapChain();
}

MGSwapChain::~MGSwapChain()
{

}

void MGSwapChain::releaseSwapChain()
{
	destroySwapchainFramebuffers();
	for (size_t i = 0; i < SwapchainImageViews.size(); i++) {
		vkDestroyImageView(OwningRenderer->LogicalDevice, SwapchainImageViews[i], nullptr);
	}
	SwapchainImages.clear();
	vkDestroySwapchainKHR(OwningRenderer->LogicalDevice, Swapchain, nullptr);
}

void MGSwapChain::initSwapChain()
{
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

	if (vkCreateSwapchainKHR(OwningRenderer->LogicalDevice, &createInfo, nullptr, &Swapchain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	SwapchainImageFormat = surfaceFormat.format;
	SwapchainExtent = extent;

	vkGetSwapchainImagesKHR(OwningRenderer->LogicalDevice, Swapchain, &imageCount, nullptr);
	SwapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(OwningRenderer->LogicalDevice, Swapchain, &imageCount, SwapchainImages.data());

	SwapchainImageViews.resize(SwapchainImages.size());

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.layerCount = 1;

	for (uint32_t i = 0; i < SwapchainImages.size(); i++) {
		SwapchainImageViews[i] = OwningRenderer->createImageView2D(SwapchainImages[i], SwapchainImageFormat, subresourceRange);
	}
}

void MGSwapChain::createSwapchainFramebuffers(VkRenderPass renderPass, bool enableDepth)
{

	destroySwapchainFramebuffers();
	if (enableDepth)
	{
		VkFormat depthFormat = OwningRenderer->findSupportedImageFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
		OwningRenderer->createImage(SwapchainExtent.width, SwapchainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, DepthTexture.image, DepthTexture.imageMemory);
		VkImageSubresourceRange range = {};
		range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		range.baseMipLevel = 0;
		range.levelCount = 1;
		range.baseArrayLayer = 0;
		range.layerCount = 1;
		DepthTexture.imageView = OwningRenderer->createImageView2D(DepthTexture.image, depthFormat, range);

		OwningRenderer->transitionImageLayout(DepthTexture.image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}
	SwapchainFramebuffers.resize(SwapchainImageViews.size());
	for (size_t i = 0; i < SwapchainImageViews.size(); i++)
	{
		std::vector<VkImageView> attachments = {};
		attachments.push_back(SwapchainImageViews[i]);
		if (enableDepth)
		{
			attachments.push_back(DepthTexture.imageView);
		}
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = SwapchainExtent.width;
		framebufferInfo.height = SwapchainExtent.height;
		framebufferInfo.layers = 1;

		MGCheckVKResultERR(vkCreateFramebuffer(OwningRenderer->LogicalDevice, &framebufferInfo, nullptr, &SwapchainFramebuffers[i]), "Swapchain Framebuffer create failed!");
	}
}

void MGSwapChain::destroySwapchainFramebuffers()
{
	if (SwapchainFramebuffers.size() > 0)
	{
		for (size_t i = 0; i < SwapchainFramebuffers.size(); i++)
		{
			vkDestroyFramebuffer(OwningRenderer->LogicalDevice, SwapchainFramebuffers[i], nullptr);
		}
		SwapchainFramebuffers.clear();
		if (DepthTexture.image)
		{
			vkDestroyImageView(OwningRenderer->LogicalDevice, DepthTexture.imageView, nullptr);
			vkDestroyImage(OwningRenderer->LogicalDevice, DepthTexture.image, nullptr);
			vkFreeMemory(OwningRenderer->LogicalDevice, DepthTexture.imageMemory, nullptr);
		}
	}
}

VkFramebuffer MGSwapChain::getSwapchainFramebuffer(int index)
{
	return SwapchainFramebuffers[index];
}

uint32_t MGSwapChain::getSwapchainImageSize()
{
	return SwapchainImages.size();
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
