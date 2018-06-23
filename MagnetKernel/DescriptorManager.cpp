#include "DescriptorManager.h"
#include "MGRenderer.h"
#include <vector>


DescriptorManager::DescriptorManager(MGRenderer* renderer)
{
	OwningRenderer = renderer;
}


DescriptorManager::~DescriptorManager()
{
}

void DescriptorManager::initManager()
{
	_createDescriptorPool();
}

void DescriptorManager::releaseManager()
{
	vkDestroyDescriptorPool(OwningRenderer->LogicalDevice, descriptorPool, nullptr);
	descriptorSets.clear();
	//std::vector<VkDescriptorSetLayout>::iterator itr;
	for (auto itr = descriptorSetLayouts.begin(); itr != descriptorSetLayouts.end(); ++itr) 
	{
		vkDestroyDescriptorSetLayout(OwningRenderer->LogicalDevice, *itr, nullptr);
	}
	descriptorSetLayouts.clear();
}

void DescriptorManager::addDescriptorSetLayout(MGConfig::MGDataBindingType * bindings, uint32_t binding_size)
{
	std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
	for (uint32_t i = 0; i < binding_size; ++i)
	{
		VkDescriptorSetLayoutBinding layout_binding = {};
		switch (bindings[i]) 
		{
		case MGConfig::MGDataBindingType::DATA_BINDING_TYPE_COMBINED_IMAGE:
			//漫反射贴图的绑定接口
			layout_binding.binding = i;
			layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;//imageSampler
			layout_binding.descriptorCount = 1;
			layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;//使用此image的着色器
			layout_binding.pImmutableSamplers = nullptr;
			break;
		default:
			break;
		}
		layout_bindings.push_back(layout_binding);
	}
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(layout_bindings.size());
	layoutInfo.pBindings = layout_bindings.data();

	VkDescriptorSetLayout descriptorSetLayout;
	if (vkCreateDescriptorSetLayout(OwningRenderer->LogicalDevice, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
	descriptorSetLayouts.push_back(descriptorSetLayout);
}

VkDescriptorSet* DescriptorManager::allocateDescriptorSet(uint32_t LayoutType, uint32_t SetCount)
{
#ifdef _DEBUG
	if (descriptorSetLayouts.size() <= LayoutType)
	{
		MGThrowError(true, "LayoutType not exist");
	}
#endif
	VkDescriptorSetLayout* layouts = new VkDescriptorSetLayout[SetCount];
	for (uint32_t i = 0; i < SetCount; ++i)
	{
		layouts[i] = descriptorSetLayouts[LayoutType];
	}
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = SetCount;
	allocInfo.pSetLayouts = layouts;
	VkDescriptorSet* sets = new VkDescriptorSet[SetCount];

	if (vkAllocateDescriptorSets(OwningRenderer->LogicalDevice, &allocInfo, sets) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor set!");
	}
	for (uint32_t i = 0; i < SetCount; ++i)
	{
		descriptorSets.push_back(sets[i]);
	}
	return sets;
}

void DescriptorManager::pushCombinedImageSampler(uint32_t dstSet, uint32_t dstBinding, Texture & tex)
{
#ifdef _DEBUG
	if (descriptorSets.size() <= dstSet)
	{
		MGThrowError(true, "Descriptor Set not exist");
	}
#endif

	VkWriteDescriptorSet write_info = {};
	write_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_info.dstSet = descriptorSets[dstSet];
	write_info.dstBinding = dstBinding;
	write_info.dstArrayElement = 0;
	write_info.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write_info.descriptorCount = 1;
	write_info.pImageInfo = tex.getPImageInfo();

	setWrites.push_back(write_info);
}

void DescriptorManager::writeDescriptorSets()
{
	vkUpdateDescriptorSets(OwningRenderer->LogicalDevice, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
	setWrites.clear();
	setWrites.shrink_to_fit();
}

void DescriptorManager::cmdBindSet(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout, uint32_t setID)
{
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[setID], 0, nullptr);
}

void DescriptorManager::_createDescriptorPool()
{
	std::vector<VkDescriptorPoolSize> poolSizes = {};
	poolSizes.resize(2);
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;////////??????????????
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 10;////////??????????????

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());;
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = 10;////////??????????????

	if (vkCreateDescriptorPool(OwningRenderer->LogicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}
