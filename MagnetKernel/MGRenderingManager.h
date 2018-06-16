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

	VkRenderPass renderPass;//�������ͼ���й�
private:
	MGRenderer* OwningRenderer;
	MGPipelineManager* pipeline_manager;


	VkDescriptorSetLayout descriptorSetLayout;//uniform���ݵĽṹ����һ��Pipeline������Щuniform����
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet>descriptorSets;

	Texture texture = {};
	Texture texture2 = {};

	MGModel* model;
	MGModelInstance* instance0;
	MGModelInstance* instance1;

	void _prepareRenderpass();//��Ȼ�����RenderPass������Attachment��ͬʱ��Pipeline������DepthStencilState,���Ҫ��Framebuffer���ж�Ӧ��ͼƬ���
	void _prepareDescriptorSetLayout();
	void _createDescriptorPool();//!
	void _allocateDescriptorSet(uint32_t SetCount, VkDescriptorSet* sets);//!

	void _prepareVerticesBuffer();
	void _prepareTextures(const char* file_path, Texture& tex);//!
	void _releaseTexture(Texture& tex);
	
	void _writeDescriptorSet(VkDescriptorSet& set, Texture& tex, Texture& NorTex);//!
};

