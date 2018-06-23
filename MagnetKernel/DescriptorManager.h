#pragma once
#include <vector>
#include "Platform.h"
#include "MGConfig.h"
#include "MGImageLoad.h"

class MGRenderer;

struct DescriptorSet
{

};

class DescriptorManager
{
public:
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;//uniform数据的结构，即一个Pipeline中有哪些uniform输入

	DescriptorManager(MGRenderer* renderer);
	~DescriptorManager();
	void initManager();
	void releaseManager();
	//void _prepareDescriptorSetLayout();
	void addDescriptorSetLayout(MGConfig::MGDataBindingType* bindings, uint32_t binding_size);
	VkDescriptorSet* allocateDescriptorSet(uint32_t LayoutType, uint32_t SetCount);//!
	void pushCombinedImageSampler(uint32_t dstSet, uint32_t dstBinding, Texture& tex);
	void writeDescriptorSets();
	void cmdBindSet(VkCommandBuffer cmdBuffer,VkPipelineLayout pipelineLayout, uint32_t setID);
private:
	MGRenderer* OwningRenderer;
	std::vector<VkWriteDescriptorSet> setWrites;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet>descriptorSets;
	void _createDescriptorPool();//!
};

