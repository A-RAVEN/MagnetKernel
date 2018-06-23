#include "MGPipelineManager.h"
#include "MGShare.h"
#include "MGRenderer.h"
#include "MGModel.h"
#include "MGModelInstance.h"
#include "MGPipeLine.h"
#include "MGMath.h"
#include "MGConfig.h"


MGPipelineManager::MGPipelineManager()
{
}

MGPipelineManager::MGPipelineManager(MGRenderer * renderer)
{
	OwningRenderer = renderer;
}

MGPipelineManager::~MGPipelineManager()
{
}

void MGPipelineManager::initManager()
{
}

void MGPipelineManager::makePipeline(VkDescriptorSetLayout * descriptorlayout, int layout_number, VkRenderPass renderpass)
{
	//��ʼ��Ĭ�Ϲ���
	pipeline = new MGPipeLine(OwningRenderer);
	pipeline->MakePipeline(descriptorlayout, layout_number, renderpass);
}

void MGPipelineManager::releaseManager()
{
	delete pipeline;
}

MGPipeLine * MGPipelineManager::GetPipeline()
{
	return pipeline;
}