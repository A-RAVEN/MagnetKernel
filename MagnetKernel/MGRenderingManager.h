#pragma once
#include <vector>
#include "Platform.h"
#include "MGImageLoad.h"


//struct MGGraphicPipeline
//{
//	VkPipeline pipeline;
//};

//class MGDescriptorManager
//{
//
//};
class MGPipelineManager;
class DescriptorManager;
class MGRenderer;
class MGModel;
class MGModelInstance;
class MGRenderingManager
{
public:
	MGRenderingManager(MGRenderer * renderer);
	~MGRenderingManager();
	void initRenderingManager();
	void deinitRenderingManager();
	void cmdExecute(VkCommandBuffer commandBuffer, int frameBufferID);

	VkRenderPass renderPass;//�������ͼ���й�
private:
	MGRenderer* OwningRenderer;
	MGPipelineManager* pipeline_manager;
	DescriptorManager* descriptor_manager;

	Texture texture = {};
	Texture texture2 = {};

	MGModel* model;
	MGModelInstance* instance0;
	MGModelInstance* instance1;

	void _prepareRenderpass();//��Ȼ�����RenderPass������Attachment��ͬʱ��Pipeline������DepthStencilState,���Ҫ��Framebuffer���ж�Ӧ��ͼƬ���

	void _prepareVerticesBuffer();
	void _prepareTextures(const char* file_path, Texture& tex);//!
	void _releaseTexture(Texture& tex);
};

