#include "MGPipelineManager.h"
#include "MGShare.h"
#include "MGRenderer.h"
#include "MGModel.h"
#include "MGModelInstance.h"
#include "MGPipeLine.h"
#include "MGMath.h"
#include "MGConfig.h"


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

	_writeDescriptorSet(descriptorSets[0],texture, texture2);
}

void MGPipelineManager::releasePipeline()
{
	model->releaseBuffers();

	vkDestroyImageView(OwningRenderer->LogicalDevice, texture.imageView, nullptr);
	vkDestroyImage(OwningRenderer->LogicalDevice, texture.image, nullptr);
	vkFreeMemory(OwningRenderer->LogicalDevice, texture.imageMemory, nullptr);

	vkDestroyImageView(OwningRenderer->LogicalDevice, texture2.imageView, nullptr);
	vkDestroyImage(OwningRenderer->LogicalDevice, texture2.image, nullptr);
	vkFreeMemory(OwningRenderer->LogicalDevice, texture2.imageMemory, nullptr);

	vkDestroyDescriptorPool(OwningRenderer->LogicalDevice, descriptorPool, nullptr);

	delete pipeline;
	vkDestroyDescriptorSetLayout(OwningRenderer->LogicalDevice, descriptorSetLayout, nullptr);
	vkDestroyRenderPass(OwningRenderer->LogicalDevice, renderPass, nullptr);
}

void MGPipelineManager::initManager()
{
}

void MGPipelineManager::makePipeline(VkDescriptorSetLayout * descriptorlayout, int layout_number, VkRenderPass renderpass)
{
	//初始化默认管线
	pipeline = new MGPipeLine(OwningRenderer);
	pipeline->MakePipeline(descriptorlayout, layout_number, renderpass);
}

void MGPipelineManager::releaseManager()
{
	delete pipeline;
}

void MGPipelineManager::cmdExecute(VkCommandBuffer commandBuffer, int frameBufferID)
{
	//renderpass
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

	//pipeline
	pipeline->CmdBindPipeline(commandBuffer);
	vkCmdSetViewport(commandBuffer, 0, 1, &OwningRenderer->createFullScreenViewport());
	vkCmdSetScissor(commandBuffer, 0, 1, &OwningRenderer->createFullScreenRect());

	//discriptorset
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipelineLayout, 0, 1, &descriptorSets[0], 0, nullptr);

	//pipeline constants
	light = mgm::vec3(50.0f, 50.0f, 50.0f);
	vkCmdPushConstants(commandBuffer, pipeline->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, MGConfig::UNIFORM_BIND_POINT_LIGHT, sizeof(mgm::vec3), &light);

	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;
	UniformViewBufferObject ubo = {};
	ubo.view = glm::lookAt(glm::vec3(0.0f, 60.0f, 40.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = mgm::perspective(glm::radians(45.0f), OwningRenderer->SwapChain->SwapchainExtent.width / (float)OwningRenderer->SwapChain->SwapchainExtent.height, 0.1f, 200.0f);
	vkCmdPushConstants(commandBuffer, pipeline->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, MGConfig::UNIFORM_BIND_POINT_CAMERA, sizeof(UniformViewBufferObject), &ubo);

	//model
	instance0->transform.setLocalRotation(time * 45.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	instance1->transform.setLocalRotation(time * 45.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	instance0->cmdDraw(commandBuffer, pipeline->pipelineLayout);
	instance1->cmdDraw(commandBuffer, pipeline->pipelineLayout);

	//renderpass
	vkCmdEndRenderPass(commandBuffer);
}

MGPipeLine * MGPipelineManager::GetPipeline()
{
	return pipeline;
}


void MGPipelineManager::_prepareRenderpass()
{
	//创建该renderpass中所有的attachment
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = OwningRenderer->SwapChain->SwapchainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

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

	std::vector<VkAttachmentDescription> attachments = { colorAttachment, depthAttachment };

	//创建绑定的引用，供subpass使用
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	//创建一个subpass
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	//创建在进入subpass之前对绑定buffer的转换操作
	VkSubpassDependency dependency = {};//
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


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
	//法线贴图的绑定接口
	VkDescriptorSetLayoutBinding samplerLayoutBindingNormal = {};
	samplerLayoutBindingNormal.binding = 0;
	samplerLayoutBindingNormal.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;//imageSampler
	samplerLayoutBindingNormal.descriptorCount = 1;
	samplerLayoutBindingNormal.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;//使用此image的着色器
	samplerLayoutBindingNormal.pImmutableSamplers = nullptr;
	//漫反射贴图的绑定接口
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
	//初始化默认管线
	pipeline = new MGPipeLine(OwningRenderer);
	pipeline->MakePipeline(&descriptorSetLayout, 1, renderPass);
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

	tex.sampler = OwningRenderer->getTextureSampler(MGConfig::MG_SAMPLER_NORMAL);
}

void MGPipelineManager::_writeDescriptorSet(VkDescriptorSet& set, Texture& tex, Texture& NorTex)
{
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