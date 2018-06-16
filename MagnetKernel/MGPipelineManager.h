#pragma once
#include "Platform.h"
#include "MGImageLoad.h"
#include "MGMath.h"
#include <vector>

class MGRenderer;

class MGModel;
class MGModelInstance;
class MGPipeLine;

//struct MGPipelineInfo
//{
//private:
//	std::vector<VkPipelineShaderStageCreateInfo> shader_stage_infos;
//	std::vector<VkDynamicState> dynamicStates;
//	VkPipelineVertexInputStateCreateInfo vertexInputInfo;
//	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
//	VkPipelineViewportStateCreateInfo viewportState;
//	VkPipelineRasterizationStateCreateInfo rasterizer;
//	VkPipelineMultisampleStateCreateInfo multisampling;
//	VkPipelineColorBlendAttachmentState colorBlendAttachment;
//	VkPipelineDepthStencilStateCreateInfo depthStencil;
//	VkPushConstantRange pushConstantRange;
//};

class MGPipelineManager
{
public:
	MGPipelineManager();
	MGPipelineManager(MGRenderer* renderer);
	~MGPipelineManager();

	VkRenderPass renderPass;//�������ͼ���й�

	void initPipeline();////outdated
	void releasePipeline();////outdated
	void initManager();
	void makePipeline(VkDescriptorSetLayout* descriptorlayout, int layout_number, VkRenderPass renderpass);
	void releaseManager();
	void cmdExecute(VkCommandBuffer commandBuffer,int frameBufferID);////outdated
	MGPipeLine* GetPipeline();
	//void updatePipeline();
	MGModel* model;////outdated
	MGModelInstance* instance0;////outdated
	MGModelInstance* instance1;////outdated
	MGPipeLine* pipeline;
private:

	MGRenderer* OwningRenderer;

	///////////////////////////////////////////////////////////////////////////////////////////////outdated
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

	void _prepareRenderpass();//��Ȼ�����RenderPass������Attachment��ͬʱ��Pipeline������DepthStencilState,���Ҫ��Framebuffer���ж�Ӧ��ͼƬ���
	void _prepareDescriptorSetLayout();//outdated
	void _prepareGraphicPipeline();

	void _createDescriptorPool();//!//outdated
	void _allocateDescriptorSet(uint32_t SetCount, VkDescriptorSet* sets);//!//outdated

	void _prepareVerticesBuffer();//outdated
	void _prepareTextures(const char* file_path, Texture& tex);//!//outdated
	//void _prepareUniformBuffer();
	void _writeDescriptorSet(VkDescriptorSet& set, Texture& tex, Texture& NorTex);//!//outdated

	VkShaderModule createShaderModule(const std::vector<char>& code);
	////////////////////////////////////////////////////////////////////////////////////outdated
};

