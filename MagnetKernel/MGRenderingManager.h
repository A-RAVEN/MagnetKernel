#pragma once
#include <vector>
#include "Platform.h"

struct MGGraphicPipeline
{
	VkPipeline pipeline;
};

class MGDescriptorManager
{

};

class MGRenderingManager
{
public:
	MGRenderingManager();
	~MGRenderingManager();
private:
	std::vector<MGGraphicPipeline> GraphicPipelines;
};

