#pragma once
#include "Platform.h"
#include "MGDebug.h"
#include "MGMath.h"
#include <vector>
#include <string>

struct CmdBuffers
{
	std::vector<VkCommandBuffer> buffers;
	std::vector<VkFence> fences;
	std::vector<bool>need_record;
	uint8_t index;
	uint8_t current_index;
	uint8_t next_index;
	bool fences_fill = false;
};

struct BufferStruct
{
	VkBuffer Buffer;
	VkDeviceMemory BufferMemory;
};

enum MGUses
{
	MG_USE_GRAPHIC,
	MG_USE_TRANSFER,
	MG_USE_COMPUTE,
	MG_USE_PRESENT
};

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static bool Equal(Vertex* vtA, Vertex* vtB)
	{
		return glm::all(glm::epsilonEqual(vtA->pos, vtB->pos, 0.00001f)) && glm::all(glm::epsilonEqual(vtA->color, vtB->color, 0.00001f)) && glm::all(glm::epsilonEqual(vtA->texCoord, vtB->texCoord, 0.00001f));
	}

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);//一个vertexdata的大小
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {};
		attributeDescriptions.resize(3);
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;//数据在shader中对应的变量
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;//数据的格式
		attributeDescriptions[1].offset = offsetof(Vertex, color);//数据在一个vertexdata中的偏移位置

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;//数据在shader中对应的变量
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;//数据的格式
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);//数据在一个vertexdata中的偏移位置

		return attributeDescriptions;
	}
};

template <class T>
struct MGArrayStruct {
	uint32_t length;
	T* data;
};

struct TransformObj {
	mgm::mat4 model;
	mgm::mat4 rotation;
	mgm::mat4 scale;
	mgm::mat4 translation;
	bool needUpdate = false;
	TransformObj* Parent = nullptr;
	TransformObj()
	{
		model = mgm::mat4(1.0f);
		rotation = mgm::mat4(1.0f);
		scale = mgm::mat4(1.0f);
		translation = mgm::mat4(1.0f);
		needUpdate = false;
		Parent = nullptr;
	}
	void relativeTranslate(mgm::vec3 move)
	{
		translation = glm::translate(translation, move);
		needUpdate = true;
	}
	void setRelativePosition(mgm::vec3 pos)
	{
		translation = glm::translate(mgm::mat4(1.0f), pos);
		needUpdate = true;
	}
	void setLocalRotation(float eulerAngle, glm::vec3 rotAxis)
	{
		rotation = glm::rotate(glm::mat4(), glm::radians(eulerAngle), rotAxis);
		needUpdate = true;
	}
	void setRelativeScale(mgm::vec3 scal)
	{
		scale = glm::scale(mgm::mat4(1.0f), scal);
		needUpdate = true;
	}
	mgm::mat4  getTransformMat()
	{
		refreshTransform();
		return model;
	}
	void refreshTransform()
	{
		if (needUpdate)
		{
			if (Parent != nullptr)
			{
				model = Parent->getTransformMat() * translation * scale * rotation;
			}
			else
			{
				model = translation * scale * rotation;
			}
		}
	}


};

struct UniformViewBufferObject {
	mgm::mat4 view;
	mgm::mat4 proj;
};

struct MGSamplers {
	VkSampler MG_SAMPLER_NORMAL;
};

struct MGSamplerInfo {
	std::string sampler_name;
	VkSamplerCreateInfo sampler_info;
};

struct MGRendererSemaphores {
	VkSemaphore MG_SEMAPHORE_SWAPCHAIN_IMAGE_AVAILABLE;
	VkSemaphore MG_SEMAPHORE_RENDER_FINISH;
	void initSemaphores(VkDevice logicalDevice) {
		VkSemaphoreCreateInfo createinfo = {};
		createinfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		MGCheckVKResultERR(vkCreateSemaphore(logicalDevice, &createinfo, nullptr, &MG_SEMAPHORE_SWAPCHAIN_IMAGE_AVAILABLE), "MG_SEMAPHORE_SWAPCHAIN_IMAGE_AVAILABLE Create Failed!");
		MGCheckVKResultERR(vkCreateSemaphore(logicalDevice, &createinfo, nullptr, &MG_SEMAPHORE_RENDER_FINISH), "MG_SEMAPHORE_RENDER_FINISH Create Failed!");
	}
	void deinitSemaphores(VkDevice logicalDevice) {
		std::vector<VkSemaphore> semaphores;
		semaphores.push_back(MG_SEMAPHORE_SWAPCHAIN_IMAGE_AVAILABLE);
		semaphores.push_back(MG_SEMAPHORE_RENDER_FINISH);
		for (auto semaphore : semaphores)
		{
			vkDestroySemaphore(logicalDevice, semaphore, nullptr);
		}
	}
};