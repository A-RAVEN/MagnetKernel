#pragma once
#include "Platform.h"
#include "MGInstance.h"
#include "MGWindow.h"
#include <vector>

class MGSwapChain;

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

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

class MGRenderer
{
public:
	MGRenderer();
	MGRenderer(MGInstance* instance,MGWindow& window);
	~MGRenderer();
	void releaseRenderer();

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	VkDevice LogicalDevice;
	MGDevice PhysicalDevice;

	int GraphicQueueFamilyIndex;
	int TransferQueueFamilyIndex;
	int ComputeQueueFamilyIndex;
	int PresentQueueFamilyIndex;


private:

	VkSurfaceKHR OutputSurface;

	//在CommandPools中的Indices
	int GraphicCommandPoolIndex;
	int TransferCommandPoolIndex;
	int ComputeCommandPoolIndex;
	int PresentCommandPoolIndex;

	//在ActiveQueues中的Indices
	std::vector<int>GraphicQueuesIndices;
	std::vector<int>TransferQueuesIndices;
	std::vector<int>ComputeQueuesIndices;
	std::vector<int>PresentQueuesIndices;

	std::vector<VkQueue>ActiveQueues;
	std::vector<VkCommandPool>CommandPools;

	MGSwapChain* SwapChain;

	void _selectPhysicalDevice(MGInstance* instance, VkSurfaceKHR windowSurface);
	void _initLogicalDevice();
	void _initCommandPools();
	void _deInitCommandPools();

	//////////////////////////测试基本的绘图功能，日后可能封装到其它类中
	VkRenderPass renderPass;//与输出的图像有关
	VkPipelineLayout pipelineLayout;//与输入的uniform数据有关
	VkDescriptorSetLayout descriptorSetLayout;//uniform数据的结构，即一个Pipeline中有哪些uniform输入
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;//由uniform数据Buffer和ImageView&Sampler组成的对象，需要与SetLayout所描述的结构相对应
	VkPipeline graphicsPipeline;

	void _prepareRenderpass();
	void _prepareDescriptorSetLayout();
	void _prepareGraphicPipeline();

	VkShaderModule createShaderModule(const std::vector<char>& code);
};

class MGSwapChain {
public:
	MGSwapChain(MGRenderer* renderer, MGWindow* window);
	~MGSwapChain();
	void releaseSwapChain();

	VkSwapchainKHR swapchain;

	VkFormat SwapchainImageFormat;
	VkExtent2D SwapchainExtent;

private:
	MGRenderer* OwningRenderer;
	MGWindow* RelatingWindow;

	std::vector<VkImage>SwapchainImages;
	std::vector<VkImageView> SwapchainImageViews;

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities, int actualWidth, int actualHeight);
};