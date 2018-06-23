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

	VkRenderPass renderPass;//与输出的图像有关
private:
	MGRenderer* OwningRenderer;
	MGPipelineManager* pipeline_manager;
	DescriptorManager* descriptor_manager;

	Texture texture = {};
	Texture texture2 = {};

	MGModel* model;
	MGModelInstance* instance0;
	MGModelInstance* instance1;

	void _prepareRenderpass();//深度缓冲在RenderPass中设置Attachment，同时在Pipeline中设置DepthStencilState,最后还要在Framebuffer中有对应的图片输出

	void _prepareVerticesBuffer();
	void _prepareTextures(const char* file_path, Texture& tex);//!
	void _releaseTexture(Texture& tex);
};

