#include "MGRenderingManager.h"
#include "MGRenderer.h"
#include "MGModel.h"
#include "MGModelInstance.h"
#include "MGPipelineManager.h"
#include "MGPipeline.h"
#include "DescriptorManager.h"


MGRenderingManager::MGRenderingManager(MGRenderer * renderer)
{
	OwningRenderer = renderer;
	pipeline_manager = nullptr;
}


MGRenderingManager::~MGRenderingManager()
{
}

void MGRenderingManager::initRenderingManager()
{
	_prepareRenderpass();
	//_prepareDescriptorSetLayout();//old

	descriptor_manager = new DescriptorManager(OwningRenderer);
	descriptor_manager->initManager();
	MGConfig::MGDataBindingType types[] = { MGConfig::DATA_BINDING_TYPE_COMBINED_IMAGE, MGConfig::DATA_BINDING_TYPE_COMBINED_IMAGE };
	descriptor_manager->addDescriptorSetLayout(&types[0], 2);

	//初始化pipeline manager
	pipeline_manager = new MGPipelineManager(OwningRenderer);
	pipeline_manager->makePipeline(&(descriptor_manager->descriptorSetLayouts[0]), 1, renderPass);


	descriptor_manager->allocateDescriptorSet(0, 1);

	//_createDescriptorPool();//old
	//descriptorSets.resize(2);//old
	//_allocateDescriptorSet(descriptorSets.size(), descriptorSets.data());//old

	_prepareVerticesBuffer();
	_prepareTextures("textures/Grey.bmp", texture);
	_prepareTextures("textures/EarthNor.bmp", texture2);

	descriptor_manager->pushCombinedImageSampler(0, 0, texture2);
	descriptor_manager->pushCombinedImageSampler(0, 1, texture);
	descriptor_manager->writeDescriptorSets();
	//_writeDescriptorSet(descriptorSets[0], texture, texture2);//old
}

void MGRenderingManager::deinitRenderingManager()
{
	model->releaseBuffers();

	_releaseTexture(texture);
	_releaseTexture(texture2);


	//vkDestroyDescriptorPool(OwningRenderer->LogicalDevice, descriptorPool, nullptr);

	//释放pipeline manager
	pipeline_manager->releaseManager();
	delete pipeline_manager;

	descriptor_manager->releaseManager();
	delete descriptor_manager;

	//vkDestroyDescriptorSetLayout(OwningRenderer->LogicalDevice, descriptorSetLayout, nullptr);
	vkDestroyRenderPass(OwningRenderer->LogicalDevice, renderPass, nullptr);
}

void MGRenderingManager::cmdExecute(VkCommandBuffer commandBuffer, int frameBufferID)
{
	//begin renderpass
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

	//rendering processes
	///start pipeline
	(pipeline_manager->GetPipeline())->CmdBindPipeline(commandBuffer);
	vkCmdSetViewport(commandBuffer, 0, 1, &OwningRenderer->createFullScreenViewport());
	vkCmdSetScissor(commandBuffer, 0, 1, &OwningRenderer->createFullScreenRect());
	///inputs
	////descriptors
	//vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (pipeline_manager->GetPipeline())->pipelineLayout, 0, 1, &descriptorSets[0], 0, nullptr);//old
	descriptor_manager->cmdBindSet(commandBuffer, (pipeline_manager->GetPipeline())->pipelineLayout, 0);
	////push constants
	mgm::vec3 light = mgm::vec3(50.0f, 50.0f, 50.0f);
	vkCmdPushConstants(commandBuffer, (pipeline_manager->GetPipeline())->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, MGConfig::UNIFORM_BIND_POINT_LIGHT, sizeof(mgm::vec3), &light);
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;
	UniformViewBufferObject ubo = {};
	ubo.view = glm::lookAt(glm::vec3(0.0f, 60.0f, 40.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = mgm::perspective(glm::radians(45.0f), OwningRenderer->SwapChain->SwapchainExtent.width / (float)OwningRenderer->SwapChain->SwapchainExtent.height, 0.1f, 200.0f);
	vkCmdPushConstants(commandBuffer, (pipeline_manager->GetPipeline())->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, MGConfig::UNIFORM_BIND_POINT_CAMERA, sizeof(UniformViewBufferObject), &ubo);

	///draw mesh
	instance0->transform.setLocalRotation(time * 45.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	instance1->transform.setLocalRotation(time * 45.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	instance0->cmdDraw(commandBuffer, (pipeline_manager->GetPipeline())->pipelineLayout);
	instance1->cmdDraw(commandBuffer, (pipeline_manager->GetPipeline())->pipelineLayout);

	//end renderpass
	vkCmdEndRenderPass(commandBuffer);
}

void MGRenderingManager::_prepareRenderpass()
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


void MGRenderingManager::_prepareVerticesBuffer()
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

void MGRenderingManager::_prepareTextures(const char * file_path, Texture & tex)
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

void MGRenderingManager::_releaseTexture(Texture & tex)
{
	vkDestroyImageView(OwningRenderer->LogicalDevice, tex.imageView, nullptr);
	vkDestroyImage(OwningRenderer->LogicalDevice, tex.image, nullptr);
	vkFreeMemory(OwningRenderer->LogicalDevice, tex.imageMemory, nullptr);
}
