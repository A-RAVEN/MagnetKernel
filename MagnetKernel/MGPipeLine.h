#pragma once
#include <vector>
#include "Platform.h"
class MGRenderer;
class MGPipeLine
{
public:
	MGRenderer* OwningRenderer;
	VkPipelineLayout pipelineLayout;
	MGPipeLine(MGRenderer* renderer);
	~MGPipeLine();
	void MakePipeline(VkDescriptorSetLayout* DescritorSetLayouts, uint32_t DescriptorSetLayoutsNum, VkRenderPass renderPass);
	void ReleasePipeline();
	void CmdBindPipeline(VkCommandBuffer cmdBuffer);
	VkShaderModule createShaderModule(const std::vector<char>& code);
private:
	VkPipeline pipeline;

};

