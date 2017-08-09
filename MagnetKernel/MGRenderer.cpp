#include "MGMath.h"
#include "MGDebug.h"
#include "MGRenderer.h"
#include "MGImageFunctions.h"
#include "MGShare.h"



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

	depthAttachment.format = mgFindSupportedImageFormat(
		PhysicalDevice.device,
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
	MGCheckVKResultERR(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline),"Pipeline创建失败");

	//.......

	vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
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
