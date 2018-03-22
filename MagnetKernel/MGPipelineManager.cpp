#include "MGPipelineManager.h"
#include "MGShare.h"
#include "MGRenderer.h"
#include "MGModel.h"
#include "MGModelInstance.h"
#include "MGPipeLine.h"
#include "MGMath.h"
#include "MGConfig.h"

//const std::vector<uint16_t> indices = {
//	0, 1, 2, 2, 3, 0,
//	4, 5, 6, 6, 7, 4
//};


MGPipelineManager::MGPipelineManager()
{
}

MGPipelineManager::MGPipelineManager(MGRenderer * renderer)
{
	OwningRenderer = renderer;
}


MGPipelineManager::~MGPipelineManager()
{
}

void MGPipelineManager::initPipeline()
{
	_prepareRenderpass();
	_prepareDescriptorSetLayout();
	_prepareGraphicPipeline();

	_createDescriptorPool();
	descriptorSets.resize(2);
	_allocateDescriptorSet(descriptorSets.size(), descriptorSets.data());

	_prepareVerticesBuffer();
	_prepareTextures("textures/Grey.bmp", texture);
	_prepareTextures("textures/EarthNor.bmp", texture2);
	_prepareUniformBuffer();

	_writeDescriptorSet(descriptorSets[0],texture, texture2);
	//_writeDescriptorSet(descriptorSets[1], texture2);
}

void MGPipelineManager::releasePipeline()
{
	model->releaseBuffers();
	//vkDestroyBuffer(OwningRenderer->LogicalDevice, vertexBuffer, nullptr);
	//vkFreeMemory(OwningRenderer->LogicalDevice, vertexBufferMemory, nullptr);
	//vkDestroyBuffer(OwningRenderer->LogicalDevice, indexBuffer, nullptr);
	//vkFreeMemory(OwningRenderer->LogicalDevice, indexBufferMemory, nullptr);
	vkDestroyBuffer(OwningRenderer->LogicalDevice, uniformBuffer, nullptr);
	vkFreeMemory(OwningRenderer->LogicalDevice, uniformBufferMemory, nullptr);

	vkDestroyImageView(OwningRenderer->LogicalDevice, texture.imageView, nullptr);
	vkDestroyImage(OwningRenderer->LogicalDevice, texture.image, nullptr);
	vkFreeMemory(OwningRenderer->LogicalDevice, texture.imageMemory, nullptr);

	vkDestroyImageView(OwningRenderer->LogicalDevice, texture2.imageView, nullptr);
	vkDestroyImage(OwningRenderer->LogicalDevice, texture2.image, nullptr);
	vkFreeMemory(OwningRenderer->LogicalDevice, texture2.imageMemory, nullptr);

	vkDestroyDescriptorPool(OwningRenderer->LogicalDevice, descriptorPool, nullptr);
	//vkDestroyPipelineLayout(OwningRenderer->LogicalDevice, pipeline->pipelineLayout, nullptr);
	//vkDestroyPipeline(OwningRenderer->LogicalDevice, graphicsPipeline, nullptr);//Deprecated
	delete pipeline;
	vkDestroyDescriptorSetLayout(OwningRenderer->LogicalDevice, descriptorSetLayout, nullptr);
	vkDestroyRenderPass(OwningRenderer->LogicalDevice, renderPass, nullptr);
}

void MGPipelineManager::cmdExecute(VkCommandBuffer commandBuffer, int frameBufferID)
{
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = OwningRenderer->SwapChain->getSwapchainFramebuffer(frameBufferID);
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = OwningRenderer->SwapChain->SwapchainExtent;

	std::vector<VkClearValue> clearValues = {};
	clearValues.resize(2);
	clearValues[0].color = { 0.0f, 0.4f, 1.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);


	//vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);////Deprecated
	pipeline->CmdBindPipeline(commandBuffer);
	vkCmdSetViewport(commandBuffer, 0, 1, &OwningRenderer->createFullScreenViewport());
	vkCmdSetScissor(commandBuffer, 0, 1, &OwningRenderer->createFullScreenRect());

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipelineLayout, 0, 1, &descriptorSets[0], 0, nullptr);

	light = mgm::vec3(50.0f, 50.0f, 50.0f);
	vkCmdPushConstants(commandBuffer, pipeline->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, MGConfig::UNIFORM_BIND_POINT_LIGHT, sizeof(mgm::vec3), &light);

	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;
	UniformBufferObject ubo = {};
	ubo.view = glm::lookAt(glm::vec3(0.0f, 60.0f, 40.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = mgm::perspective(glm::radians(45.0f), OwningRenderer->SwapChain->SwapchainExtent.width / (float)OwningRenderer->SwapChain->SwapchainExtent.height, 0.1f, 200.0f);
	vkCmdPushConstants(commandBuffer, pipeline->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, MGConfig::UNIFORM_BIND_POINT_CAMERA, sizeof(UniformBufferObject), &ubo);

	instance0->transform.setLocalRotation(time * 45.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	instance1->transform.setLocalRotation(time * 45.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	instance0->cmdDraw(commandBuffer, pipeline->pipelineLayout);
	instance1->cmdDraw(commandBuffer, pipeline->pipelineLayout);

	vkCmdEndRenderPass(commandBuffer);
}

//void MGPipelineManager::updatePipeline()
//{
//	static auto startTime = std::chrono::high_resolution_clock::now();
//
//	auto currentTime = std::chrono::high_resolution_clock::now();
//	float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;
//
//	UniformBufferObject ubo = {};
//	//ubo.model = glm::rotate(glm::mat4(), time * glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
//	ubo.view = glm::lookAt(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
//	ubo.proj = mgm::perspective(glm::radians(45.0f), OwningRenderer->SwapChain->SwapchainExtent.width / (float)OwningRenderer->SwapChain->SwapchainExtent.height, 0.1f, 200.0f);
//	//ubo.proj = glm::perspective(glm::radians(45.0f), 800.0f/600.0f, 0.1f, 10.0f);
//	//ubo.proj[1][1] *= -1;
//
//	void* data;
//	vkMapMemory(OwningRenderer->LogicalDevice, uniformBufferMemory, 0, sizeof(ubo), 0, &data);
//	memcpy(data, &ubo, sizeof(ubo));
//	vkUnmapMemory(OwningRenderer->LogicalDevice, uniformBufferMemory);
//}

void MGPipelineManager::_prepareRenderpass()
{
	//mgCreateColorAttachment_SwapchainPresent(VkFormat swapchainFormat)
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = OwningRenderer->SwapChain->SwapchainImageFormat;
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

	depthAttachment.format = OwningRenderer->findSupportedImageFormat(
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

	MGCheckVKResultERR(vkCreateRenderPass(OwningRenderer->LogicalDevice, &renderPassInfo, nullptr, &renderPass), "RenderPass创建失败！");

}

void MGPipelineManager::_prepareDescriptorSetLayout()
{
	//VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	//uboLayoutBinding.binding = 0;//binding used in shader
	//uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;//uniformbuffer
	//uboLayoutBinding.descriptorCount = 1;
	//uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;//使用此uniform的着色器
	//uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutBinding samplerLayoutBindingNormal = {};
	samplerLayoutBindingNormal.binding = 0;
	samplerLayoutBindingNormal.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;//imageSampler
	samplerLayoutBindingNormal.descriptorCount = 1;
	samplerLayoutBindingNormal.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;//使用此image的着色器
	samplerLayoutBindingNormal.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;//imageSampler
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;//使用此image的着色器
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { samplerLayoutBindingNormal, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(OwningRenderer->LogicalDevice, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void MGPipelineManager::_prepareGraphicPipeline()
{
	pipeline = new MGPipeLine(OwningRenderer);
	pipeline->MakePipeline(&descriptorSetLayout, 1, renderPass);
	//auto vertShaderCode = mgReadFile("Shaders/push_const/vert.spv");
	//auto fragShaderCode = mgReadFile("Shaders/push_const/frag.spv");

	//VkShaderModule vertShaderModule;
	//VkShaderModule fragShaderModule;

	//vertShaderModule = createShaderModule(vertShaderCode);
	//fragShaderModule = createShaderModule(fragShaderCode);

	////建立顶点着色级别
	//VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	//vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	//vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	//vertShaderStageInfo.module = vertShaderModule;
	//vertShaderStageInfo.pName = "main";

	////建立片元着色级别
	//VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	//fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	//fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	//fragShaderStageInfo.module = fragShaderModule;
	//fragShaderStageInfo.pName = "main";

	//VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	////FixedFunctions
	//VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	//auto bindingDescription = Vertex::getBindingDescription();
	//auto attributeDescriptions = Vertex::getAttributeDescriptions();
	//vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	//vertexInputInfo.vertexBindingDescriptionCount = 1;
	//vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; // Optional
	//vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	//vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // Optional

	//VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	//inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	//inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	//inputAssembly.primitiveRestartEnable = VK_FALSE;

	//////////////////////////////////////////////活动模块
	//VkViewport viewport = {};
	//viewport.x = 0.0f;
	//viewport.y = 0.0f;
	//viewport.width = (float)OwningRenderer->SwapChain->SwapchainExtent.width;
	//viewport.height = (float)OwningRenderer->SwapChain->SwapchainExtent.height;
	//viewport.minDepth = 0.0f;
	//viewport.maxDepth = 1.0f;

	//VkRect2D scissor = {};
	//scissor.offset = { 0, 0 };
	//scissor.extent = OwningRenderer->SwapChain->SwapchainExtent;

	//VkPipelineViewportStateCreateInfo viewportState = {};
	//viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	//viewportState.viewportCount = 1;
	//viewportState.pViewports = &viewport;
	//viewportState.scissorCount = 1;
	//viewportState.pScissors = &scissor;
	///////////////////////////////////////////

	//VkPipelineRasterizationStateCreateInfo rasterizer = {};
	//rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	//rasterizer.depthClampEnable = VK_FALSE;
	//rasterizer.rasterizerDiscardEnable = VK_FALSE;
	//rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	//rasterizer.lineWidth = 1.0f;
	//rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	//rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	//rasterizer.depthBiasEnable = VK_FALSE;
	//rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	//rasterizer.depthBiasClamp = 0.0f; // Optional
	//rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	//VkPipelineMultisampleStateCreateInfo multisampling = {};
	//multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	//multisampling.sampleShadingEnable = VK_FALSE;
	//multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	//multisampling.minSampleShading = 1.0f; // Optional
	//multisampling.pSampleMask = nullptr; // Optional
	//multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	//multisampling.alphaToOneEnable = VK_FALSE; // Optional

	//VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	//colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	//colorBlendAttachment.blendEnable = VK_FALSE;
	//colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	//colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	//colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	//colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	//colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	//colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	//VkPipelineColorBlendStateCreateInfo colorBlending = {};
	//colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	//colorBlending.logicOpEnable = VK_FALSE;
	//colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	//colorBlending.attachmentCount = 1;
	//colorBlending.pAttachments = &colorBlendAttachment;
	//colorBlending.blendConstants[0] = 0.0f; // Optional
	//colorBlending.blendConstants[1] = 0.0f; // Optional
	//colorBlending.blendConstants[2] = 0.0f; // Optional
	//colorBlending.blendConstants[3] = 0.0f; // Optional

	//VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	//depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	//depthStencil.depthTestEnable = VK_TRUE;
	//depthStencil.depthWriteEnable = VK_TRUE;//绘制透明物体时可设为false，以防止后面的透明物体不绘出
	//depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	//depthStencil.depthBoundsTestEnable = VK_FALSE;
	//depthStencil.minDepthBounds = 0.0f; // Optional
	//depthStencil.maxDepthBounds = 1.0f; // Optional
	//depthStencil.stencilTestEnable = VK_FALSE;
	//depthStencil.front = {}; // Optional
	//depthStencil.back = {}; // Optional

	//VkPushConstantRange pushConstantRange = {};
	//pushConstantRange.offset = 0;
	//pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	//pushConstantRange.size = OwningRenderer->UNIFORM_BIND_POINT_LIGHT + sizeof(mgm::vec3);

	////Pipeline Layout,定义输入的uniform变量
	//VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	//pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	//pipelineLayoutInfo.setLayoutCount = 1; // Optional
	//pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Optional
	//pipelineLayoutInfo.pushConstantRangeCount = 1; // Optional
	//pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange; // Optional

	//MGCheckVKResultERR(vkCreatePipelineLayout(OwningRenderer->LogicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout), "PipelineLayout创建失败！");

	//std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT ,VK_DYNAMIC_STATE_SCISSOR };
	//VkPipelineDynamicStateCreateInfo pipelineDynamicStateInfo = {};
	//pipelineDynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	//pipelineDynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	//pipelineDynamicStateInfo.pDynamicStates = dynamicStates.data();

	//VkGraphicsPipelineCreateInfo pipelineInfo = {};
	//pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	//pipelineInfo.stageCount = 2;
	//pipelineInfo.pStages = shaderStages;
	//pipelineInfo.pVertexInputState = &vertexInputInfo;
	//pipelineInfo.pInputAssemblyState = &inputAssembly;
	//pipelineInfo.pViewportState = &viewportState;
	//pipelineInfo.pRasterizationState = &rasterizer;
	//pipelineInfo.pMultisampleState = &multisampling;
	//pipelineInfo.pDepthStencilState = &depthStencil; // Optional
	//pipelineInfo.pColorBlendState = &colorBlending;
	//pipelineInfo.pDynamicState = nullptr; // Optional
	//pipelineInfo.layout = pipelineLayout;
	//pipelineInfo.renderPass = renderPass;
	//pipelineInfo.subpass = 0;
	//pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	//pipelineInfo.basePipelineIndex = -1; // Optional
	//MGCheckVKResultERR(vkCreateGraphicsPipelines(OwningRenderer->LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline), "Pipeline创建失败");

	////.......

	//vkDestroyShaderModule(OwningRenderer->LogicalDevice, fragShaderModule, nullptr);
	//vkDestroyShaderModule(OwningRenderer->LogicalDevice, vertShaderModule, nullptr);
}

void MGPipelineManager::_createDescriptorPool()
{
	std::vector<VkDescriptorPoolSize> poolSizes = {};
	poolSizes.resize(2);
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;////////??????????????
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 10;////////??????????????

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());;
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = 10;////////??????????????

	if (vkCreateDescriptorPool(OwningRenderer->LogicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void MGPipelineManager::_allocateDescriptorSet(uint32_t SetCount, VkDescriptorSet* sets)
{
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout,descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = SetCount;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(OwningRenderer->LogicalDevice, &allocInfo, sets) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor set!");
	}
}



VkShaderModule MGPipelineManager::createShaderModule(const std::vector<char>& code) {
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(OwningRenderer->LogicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}

	return shaderModule;
}

void MGPipelineManager::_prepareVerticesBuffer()
{
	model = new MGModel(OwningRenderer);
	model->loadOBJ("../Assets/EarthMain.obj");
	model->buildBuffers();
	instance0 = new MGModelInstance(model);
	instance1 = new MGModelInstance(model);
	instance0->transform.relativeTranslate(glm::vec3(5.0f, 0.0f, 0.0f));
	instance0->transform.setRelativeScale(glm::vec3(2.0f, 2.0f, 2.0f));
	instance1->transform.relativeTranslate(glm::vec3(-10.0f, 0.0f, 0.0f));
	instance1->transform.setRelativeScale(glm::vec3(0.5f, 0.5f, 0.5f));
	instance1->transform.Parent = &instance0->transform;
}

void MGPipelineManager::_prepareTextures(const char* file_path, Texture& tex)
{
	MGRawImage img = mgLoadRawImage(file_path);
	if (!img.pixels)
	{
		throw std::runtime_error("failed to load texture image!");
	}
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	OwningRenderer->createBuffer(img.getImageSize(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
	void* data;
	vkMapMemory(OwningRenderer->LogicalDevice, stagingBufferMemory, 0, img.getImageSize(), 0, &data);
	memcpy(data, img.pixels, static_cast<size_t>(img.getImageSize()));
	vkUnmapMemory(OwningRenderer->LogicalDevice, stagingBufferMemory);
	stbi_image_free(img.pixels);

	OwningRenderer->createImage(img.texWidth, img.texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tex.image, tex.imageMemory);

	OwningRenderer->transitionImageLayout(tex.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	OwningRenderer->copyBufferToImage(stagingBuffer, tex.image, static_cast<uint32_t>(img.texWidth), static_cast<uint32_t>(img.texHeight));

	OwningRenderer->transitionImageLayout(tex.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(OwningRenderer->LogicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(OwningRenderer->LogicalDevice, stagingBufferMemory, nullptr);
	VkImageSubresourceRange range = {};
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.levelCount = 1;
	range.baseArrayLayer = 0;
	range.layerCount = 1;
	tex.imageView = OwningRenderer->createImageView2D(tex.image, VK_FORMAT_R8G8B8A8_UNORM, range);

	tex.sampler = OwningRenderer->TextureSamplers.MG_SAMPLER_NORMAL;
}

void MGPipelineManager::_prepareUniformBuffer()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	OwningRenderer->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, uniformBufferMemory);
}

void MGPipelineManager::_writeDescriptorSet(VkDescriptorSet& set, Texture& tex, Texture& NorTex)
{

	//VkDescriptorBufferInfo bufferInfo = {};
	//bufferInfo.buffer = uniformBuffer;
	//bufferInfo.offset = 0;
	//bufferInfo.range = sizeof(UniformBufferObject);


	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = tex.imageView;
	imageInfo.sampler = tex.sampler;

	VkDescriptorImageInfo NorimageInfo = {};
	NorimageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	NorimageInfo.imageView = NorTex.imageView;
	NorimageInfo.sampler = NorTex.sampler;

	std::vector<VkWriteDescriptorSet> descriptorWrites = {};
	descriptorWrites.resize(2);
	//descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	//descriptorWrites[0].dstSet = descriptorSet;
	//descriptorWrites[0].dstBinding = 0;
	//descriptorWrites[0].dstArrayElement = 0;
	//descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	//descriptorWrites[0].descriptorCount = 1;
	//descriptorWrites[0].pBufferInfo = &bufferInfo;

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = set;
	descriptorWrites[0].dstBinding = 1;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pImageInfo = &imageInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = set;
	descriptorWrites[1].dstBinding = 0;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &NorimageInfo;

	vkUpdateDescriptorSets(OwningRenderer->LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}