#include "MGMath.h"
#include "MGDebug.h"
#include "MGRenderer.h"
#include "MGImageLoad.h"
#include "MGShare.h"

const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0,
	4, 5, 6, 6, 7, 4
};

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
	_initSamplers();
	_initSemaphores();

	_prepareRenderpass();
	_prepareDescriptorSetLayout();
	_prepareGraphicPipeline();

	_createDescriptorPool();//!
	SwapChain->createSwapchainFramebuffers(renderPass, true);
	_allocateDescriptorSet();//!

	_prepareVerticesBuffer();
	_prepareTextures();//!
	_prepareUniformBuffer();

	_writeDescriptorSet();//!

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
	vkQueueWaitIdle(ActiveQueues[GraphicQueuesIndices[0]]);

	vkDestroyBuffer(LogicalDevice, vertexBuffer, nullptr);
	vkFreeMemory(LogicalDevice, vertexBufferMemory, nullptr);
	vkDestroyBuffer(LogicalDevice, indexBuffer, nullptr);
	vkFreeMemory(LogicalDevice, indexBufferMemory, nullptr);
	vkDestroyBuffer(LogicalDevice, uniformBuffer, nullptr);
	vkFreeMemory(LogicalDevice, uniformBufferMemory, nullptr);

	vkDestroyImageView(LogicalDevice, texture.imageView,nullptr);
	vkDestroyImage(LogicalDevice, texture.image, nullptr);
	vkFreeMemory(LogicalDevice, texture.imageMemory, nullptr);

	vkDestroyDescriptorPool(LogicalDevice, descriptorPool, nullptr);
	vkDestroyPipelineLayout(LogicalDevice, pipelineLayout, nullptr);
	vkDestroyPipeline(LogicalDevice, graphicsPipeline, nullptr);
	vkDestroyDescriptorSetLayout(LogicalDevice, descriptorSetLayout, nullptr);
	vkDestroyRenderPass(LogicalDevice, renderPass, nullptr);

	_deInitSemaphores();
	_deInitSamplers();
	SwapChain->releaseSwapChain();
	delete SwapChain;
	_deInitCommandPools();
	vkDestroyDevice(LogicalDevice, nullptr);
}

void MGRenderer::updateUniforms()
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

	UniformBufferObject ubo = {};
	ubo.model = glm::rotate(glm::mat4(), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), SwapChain->SwapchainExtent.width / (float)SwapChain->SwapchainExtent.height, 0.1f, 10.0f);
	//ubo.proj = glm::perspective(glm::radians(45.0f), 800.0f/600.0f, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	void* data;
	vkMapMemory(LogicalDevice, uniformBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(LogicalDevice, uniformBufferMemory);
}

void MGRenderer::renderFrame()
{
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(LogicalDevice, SwapChain->Swapchain, (std::numeric_limits<uint64_t>::max)(), RendererSemaphores.MG_SEMAPHORE_SWAPCHAIN_IMAGE_AVAILABLE, VK_NULL_HANDLE, &imageIndex);

	//VkCommandBuffer commandBuffer = beginSingleTimeCommands(CommandPools[GraphicCommandPoolIndex]);
	

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

	MGCheckVKResultERR(vkQueueSubmit(ActiveQueues[GraphicQueuesIndices[0]], 1, &submitInfo, VK_NULL_HANDLE), "Render command buffer submit failed!");
	//endSingleTimeCommands(commandBuffer, ActiveQueues[GraphicQueuesIndices[0]], CommandPools[GraphicCommandPoolIndex]);


	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &RendererSemaphores.MG_SEMAPHORE_RENDER_FINISH;
	VkSwapchainKHR swapChains[] = { SwapChain->Swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional


	result = vkQueuePresentKHR(ActiveQueues[PresentQueuesIndices[0]], &presentInfo);
}

void MGRenderer::OnWindowResized()
{
	vkQueueWaitIdle(ActiveQueues[GraphicQueuesIndices[0]]);
	SwapChain->releaseSwapChain();
	SwapChain->initSwapChain();
	SwapChain->createSwapchainFramebuffers(renderPass, true);
	_recordPrimaryCommandBuffers();
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
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(CommandPools[GraphicCommandPoolIndex]);

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0; // Optional
	copyRegion.dstOffset = 0; // Optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(commandBuffer,ActiveQueues[GraphicQueuesIndices[0]], CommandPools[GraphicCommandPoolIndex]);
}

void MGRenderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(CommandPools[TransferCommandPoolIndex]);
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
	endSingleTimeCommands(commandBuffer,ActiveQueues[TransferQueuesIndices[0]], CommandPools[TransferCommandPoolIndex]);
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
		//for (int queueIndex = 0; queueIndex < 1; queueIndex++) {
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
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; //
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

void MGRenderer::_initPrimaryCommandBuffer()
{
	int size = SwapChain->getSwapchainImageSize();
	PrimaryCommandBuffers.resize(size);
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = CommandPools[GraphicCommandPoolIndex];
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)size;

	if (vkAllocateCommandBuffers(LogicalDevice, &allocInfo, PrimaryCommandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
	_recordPrimaryCommandBuffers();
}

void MGRenderer::_recordPrimaryCommandBuffers()
{
	for (int i = 0; i < PrimaryCommandBuffers.size(); i++) {
		VkCommandBuffer PrimaryCommandBuffer = PrimaryCommandBuffers[i];
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		vkBeginCommandBuffer(PrimaryCommandBuffer, &beginInfo);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = SwapChain->getSwapchainFramebuffer(i);
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = SwapChain->SwapchainExtent;

		std::vector<VkClearValue> clearValues = {};
		clearValues.resize(2);
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(PrimaryCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(PrimaryCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)SwapChain->SwapchainExtent.width;
		viewport.height = (float)SwapChain->SwapchainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(PrimaryCommandBuffer, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = SwapChain->SwapchainExtent;
		vkCmdSetScissor(PrimaryCommandBuffer, 0, 1, &scissor);


		//draw verticies
		VkBuffer vertexBuffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindDescriptorSets(PrimaryCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
		vkCmdBindVertexBuffers(PrimaryCommandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(PrimaryCommandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
		//vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0);
		vkCmdDrawIndexed(PrimaryCommandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
		vkCmdEndRenderPass(PrimaryCommandBuffer);

		vkEndCommandBuffer(PrimaryCommandBuffer);
	}
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

void MGRenderer::_prepareRenderpass()
{
	//mgCreateColorAttachment_SwapchainPresent(VkFormat swapchainFormat)
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = SwapChain->SwapchainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment = {};

	depthAttachment.format = findSupportedImageFormat(
	{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;


	VkSubpassDependency dependency = {};//
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::vector<VkAttachmentDescription> attachments = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	MGCheckVKResultERR(vkCreateRenderPass(LogicalDevice, &renderPassInfo, nullptr, &renderPass),"RenderPass创建失败！");

}

void MGRenderer::_prepareDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;//binding used in shader
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;//uniformbuffer
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;//使用此uniform的着色器
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;//imageSampler
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;//使用此image的着色器
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { uboLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(LogicalDevice, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void MGRenderer::_prepareGraphicPipeline()
{
	auto vertShaderCode = mgReadFile("Shaders/05/vert.spv");
	auto fragShaderCode = mgReadFile("Shaders/05/frag.spv");

	VkShaderModule vertShaderModule;
	VkShaderModule fragShaderModule;

	vertShaderModule = createShaderModule(vertShaderCode);
	fragShaderModule = createShaderModule(fragShaderCode);

	//建立顶点着色级别
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	//建立片元着色级别
	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	//FixedFunctions
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; // Optional
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // Optional

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	////////////////////////////////////////////活动模块
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)SwapChain->SwapchainExtent.width;
	viewport.height = (float)SwapChain->SwapchainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = SwapChain->SwapchainExtent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;
	/////////////////////////////////////////

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;//绘制透明物体时可设为false，以防止后面的透明物体不绘出
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional

	//Pipeline Layout,定义输入的uniform变量
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1; // Optional
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = 0; // Optional

	MGCheckVKResultERR(vkCreatePipelineLayout(LogicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout), "PipelineLayout创建失败！");

	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT ,VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo pipelineDynamicStateInfo = {};
	pipelineDynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	pipelineDynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	pipelineDynamicStateInfo.pDynamicStates = dynamicStates.data();

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil; // Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; // Optional
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
	MGCheckVKResultERR(vkCreateGraphicsPipelines(LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline),"Pipeline创建失败");

	//.......

	vkDestroyShaderModule(LogicalDevice, fragShaderModule, nullptr);
	vkDestroyShaderModule(LogicalDevice, vertShaderModule, nullptr);
}

void MGRenderer::_createDescriptorPool()
{
	std::vector<VkDescriptorPoolSize> poolSizes = {};
	poolSizes.resize(2);
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());;
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = 1;

	if (vkCreateDescriptorPool(LogicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void MGRenderer::_allocateDescriptorSet()
{
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(LogicalDevice, &allocInfo, &descriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor set!");
	}
}

void MGRenderer::_prepareVerticesBuffer()
{
	std::vector<Vertex> vertices = {
		{ { -0.5f, -0.5f, 0.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f } },
		{ { 0.5f, -0.5f, 0.0f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f } },
		{ { 0.5f, 0.5f, 0.0f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f } },
		{ { -0.5f, 0.5f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f } },

		{ { -0.5f, -0.5f, -0.5f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f } },
		{ { 0.5f, -0.5f, -0.5f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f } },
		{ { 0.5f, 0.5f, -0.5f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f } },
		{ { -0.5f, 0.5f, -0.5f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f } }
	};

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(LogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(LogicalDevice, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

	copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	vkDestroyBuffer(LogicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(LogicalDevice, stagingBufferMemory, nullptr);
	////////////////
	///////////////
	bufferSize = sizeof(indices[0]) * indices.size();

	//VkBuffer stagingBuffer;
	//VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data2;
	vkMapMemory(LogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data2);
	memcpy(data2, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(LogicalDevice, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

	copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(LogicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(LogicalDevice, stagingBufferMemory, nullptr);
}

void MGRenderer::_prepareTextures()
{
	MGRawImage img = mgLoadRawImage("textures/texture.jpg");
	if (!img.pixels)
	{
		throw std::runtime_error("failed to load texture image!");
	}
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(img.getImageSize(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
	void* data;
	vkMapMemory(LogicalDevice, stagingBufferMemory, 0, img.getImageSize(), 0, &data);
	memcpy(data, img.pixels, static_cast<size_t>(img.getImageSize()));
	vkUnmapMemory(LogicalDevice, stagingBufferMemory);
	stbi_image_free(img.pixels);

	createImage(img.texWidth, img.texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture.image, texture.imageMemory);

	transitionImageLayout(texture.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(stagingBuffer, texture.image, static_cast<uint32_t>(img.texWidth), static_cast<uint32_t>(img.texHeight));

	transitionImageLayout(texture.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(LogicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(LogicalDevice, stagingBufferMemory, nullptr);
	VkImageSubresourceRange range = {};
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.levelCount = 1;
	range.baseArrayLayer = 0;
	range.layerCount = 1;
	texture.imageView = createImageView2D(texture.image, VK_FORMAT_R8G8B8A8_UNORM, range);

	texture.sampler = TextureSamplers.MG_SAMPLER_NORMAL;
}

void MGRenderer::_prepareUniformBuffer()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, uniformBufferMemory);
}

void MGRenderer::_writeDescriptorSet()
{

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);

	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = texture.imageView;
	imageInfo.sampler = texture.sampler;

	std::vector<VkWriteDescriptorSet> descriptorWrites = {};
	descriptorWrites.resize(2);
	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

VkShaderModule MGRenderer::createShaderModule(const std::vector<char>& code) {
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(LogicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}

	return shaderModule;
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

void MGRenderer::endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue submitqueue, VkCommandPool cmdPool)
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
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(CommandPools[GraphicCommandPoolIndex]);
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
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,//barrier在此阶段开始之后执行
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,//barrier在此阶段结束之前完成
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
	endSingleTimeCommands(commandBuffer, ActiveQueues[GraphicQueuesIndices[GraphicQueuesIndices.size() - 1]], CommandPools[GraphicCommandPoolIndex]);
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

int MGSwapChain::getSwapchainImageSize()
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
