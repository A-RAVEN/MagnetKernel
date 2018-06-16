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


	VkDescriptorSetLayout descriptorSetLayout;//uniform数据的结构，即一个Pipeline中有哪些uniform输入
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet>descriptorSets;

	Texture texture = {};
	Texture texture2 = {};

	MGModel* model;
	MGModelInstance* instance0;
	MGModelInstance* instance1;

	void _prepareRenderpass();//深度缓冲在RenderPass中设置Attachment，同时在Pipeline中设置DepthStencilState,最后还要在Framebuffer中有对应的图片输出
	void _prepareDescriptorSetLayout();
	void _createDescriptorPool();//!
	void _allocateDescriptorSet(uint32_t SetCount, VkDescriptorSet* sets);//!

	void _prepareVerticesBuffer();
	void _prepareTextures(const char* file_path, Texture& tex);//!
	void _releaseTexture(Texture& tex);
	
	void _writeDescriptorSet(VkDescriptorSet& set, Texture& tex, Texture& NorTex);//!
};

