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
	void initManager();
	void makePipeline(VkDescriptorSetLayout* descriptorlayout, int layout_number, VkRenderPass renderpass);
	void releaseManager();
	MGPipeLine* GetPipeline();
	MGPipeLine* pipeline;
private:

	MGRenderer* OwningRenderer;
};

