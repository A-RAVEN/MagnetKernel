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

	VkRenderPass renderPass;//与输出的图像有关

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
	//VkPipelineLayout pipelineLayout;//与输入的uniform数据有关
	VkDescriptorSetLayout descriptorSetLayout;//uniform数据的结构，即一个Pipeline中有哪些uniform输入
	VkDescriptorPool descriptorPool;
	//VkDescriptorSet descriptorSet;//由uniform数据Buffer和ImageView&Sampler组成的对象，需要与SetLayout所描述的结构相对应
	std::vector<VkDescriptorSet>descriptorSets;
	//VkPipeline graphicsPipeline;

	Texture texture = {};
	Texture texture2 = {};
	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBufferMemory;
	mgm::vec3 light;

	void _prepareRenderpass();//深度缓冲在RenderPass中设置Attachment，同时在Pipeline中设置DepthStencilState,最后还要在Framebuffer中有对应的图片输出
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

