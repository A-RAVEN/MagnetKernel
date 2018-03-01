#include "MGModelInstance.h"
#include "MGModel.h"


MGModelInstance::MGModelInstance(MGModel* model)
{
	this->model = model;
}


MGModelInstance::~MGModelInstance()
{
}

void MGModelInstance::cmdDraw(VkCommandBuffer commandBuffer,VkPipelineLayout pipelineLayout, uint32_t modleMatOffset)
{
	if (model != nullptr)
	{
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, modleMatOffset, sizeof(transform.model), &transform.getTransformMat());
		model->MGCmdDraw(commandBuffer);
	}
}
