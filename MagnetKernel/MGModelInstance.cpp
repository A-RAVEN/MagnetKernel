#include "MGModelInstance.h"
#include "MGModel.h"
#include "MGConfig.h"


MGModelInstance::MGModelInstance(MGModel* model)
{
	this->model = model;
}


MGModelInstance::~MGModelInstance()
{
}

void MGModelInstance::cmdDraw(VkCommandBuffer commandBuffer,VkPipelineLayout pipelineLayout)
{
	if (model != nullptr)
	{
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, MGConfig::UNIFORM_BIND_POINT_MODEL_MATRIX, sizeof(transform.model), &transform.getTransformMat());
		model->MGCmdDraw(commandBuffer);
	}
}
