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

	VkRenderPass renderPass;//�������ͼ���й�
	VkPipelineLayout pipelineLayout;//�������uniform�����й�
	VkDescriptorSetLayout descriptorSetLayout;//uniform���ݵĽṹ����һ��Pipeline������Щuniform����
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;//��uniform����Buffer��ImageView&Sampler��ɵĶ�����Ҫ��SetLayout�������Ľṹ���Ӧ
	VkPipeline graphicsPipeline;

	Texture texture = {};
	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBufferMemory;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	void _prepareRenderpass();//��Ȼ�����RenderPass������Attachment��ͬʱ��Pipeline������DepthStencilState,���Ҫ��Framebuffer���ж�Ӧ��ͼƬ���
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

