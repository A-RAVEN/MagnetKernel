#pragma once
#include "MGRenderer.h"

class MGModel;
class MGModelInstance
{
public:
	MGModelInstance(MGModel* model = nullptr);
	~MGModelInstance();
	void cmdDraw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t modleMatOffset);
	TransformObj transform;
private:

	MGModel* model;

};

