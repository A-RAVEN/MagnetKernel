#pragma once
#include "Platform.h"
#include "MGImageLoad.h"
#include "MGMath.h"
#include <vector>

class MGRenderer;

class MGModel;
class MGModelInstance;
class MGPipeLine;

struct MGPipelineInfo
{
private:
	std::vector<VkPipelineShaderStageCreateInfo> shader_stage_infos;
	std::vector<VkDynamicState> dynamicStates;
	VkPipelineVertexInputStateCreateInfo vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
	VkPipelineViewportStateCreateInfo viewportState;
	VkPipelineRasterizationStateCreateInfo rasterizer;
	VkPipelineMultisampleStateCreateInfo multisampling;
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	VkPipelineDepthStencilStateCreateInfo depthStencil;
	VkPushConstantRange pushConstantRange;
};

class MGPipelineManager
{
public:
	MGPipelineManager();
	MGPipelineManager(MGRenderer* renderer);
	~MGPipelineManager();

	VkRenderPass renderPass;//�������ͼ���й�

	void initPipeline();
	void releasePipeline();
	void cmdExecute(VkCommandBuffer commandBuffer,int frameBufferID);
	//void updatePipeline();
	MGModel* model;
	MGModelInstance* instance0;
	MGModelInstance* instance1;
	MGPipeLine* pipeline;
private:

	MGRenderer* OwningRenderer;


	//VkPipelineLayout pipelineLayout;//�������uniform�����й�
	VkDescriptorSetLayout descriptorSetLayout;//uniform���ݵĽṹ����һ��Pipeline������Щuniform����
	VkDescriptorPool descriptorPool;
	//VkDescriptorSet descriptorSet;//��uniform����Buffer��ImageView&Sampler��ɵĶ�����Ҫ��SetLayout�������Ľṹ���Ӧ
	std::vector<VkDescriptorSet>descriptorSets;
	//VkPipeline graphicsPipeline;

	Texture texture = {};
	Texture texture2 = {};
	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBufferMemory;
	mgm::vec3 light;
	//VkBuffer vertexBuffer;
	//VkDeviceMemory vertexBufferMemory;
	//VkBuffer indexBuffer;
	//VkDeviceMemory indexBufferMemory;

	void _prepareRenderpass();//��Ȼ�����RenderPass������Attachment��ͬʱ��Pipeline������DepthStencilState,���Ҫ��Framebuffer���ж�Ӧ��ͼƬ���
	void _prepareDescriptorSetLayout();
	void _prepareGraphicPipeline();

	void _createDescriptorPool();//!
	void _allocateDescriptorSet(uint32_t SetCount, VkDescriptorSet* sets);//!

	void _prepareVerticesBuffer();
	void _prepareTextures(const char* file_path, Texture& tex);//!
	//void _prepareUniformBuffer();
	void _writeDescriptorSet(VkDescriptorSet& set, Texture& tex, Texture& NorTex);//!

	VkShaderModule createShaderModule(const std::vector<char>& code);
};

