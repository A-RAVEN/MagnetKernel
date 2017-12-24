#pragma once
#include "Platform.h"
#include "MGImageLoad.h"
#include <vector>

class MGRenderer;

struct MGRenderPass
{

};

class MGPipeline
{
public:
	MGPipeline();
	MGPipeline(MGRenderer* renderer);
	~MGPipeline();

	MGRenderer* OwningRenderer;

	VkRenderPass renderPass;//与输出的图像有关
	VkPipelineLayout pipelineLayout;//与输入的uniform数据有关
	VkDescriptorSetLayout descriptorSetLayout;//uniform数据的结构，即一个Pipeline中有哪些uniform输入
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;//由uniform数据Buffer和ImageView&Sampler组成的对象，需要与SetLayout所描述的结构相对应
	VkPipeline graphicsPipeline;

	Texture texture = {};
	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBufferMemory;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	void _prepareRenderpass();//深度缓冲在RenderPass中设置Attachment，同时在Pipeline中设置DepthStencilState,最后还要在Framebuffer中有对应的图片输出
	void _prepareDescriptorSetLayout();
	void _prepareGraphicPipeline();

	void _createDescriptorPool();//!
	void _allocateDescriptorSet();//!

	void _prepareVerticesBuffer();
	void _prepareTextures();//!
	void _prepareUniformBuffer();
	void _writeDescriptorSet();//!

	VkShaderModule createShaderModule(const std::vector<char>& code);
};

