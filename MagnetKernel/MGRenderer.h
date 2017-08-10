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
		bindingDescription.stride = sizeof(Vertex);//һ��vertexdata�Ĵ�С
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
		attributeDescriptions[1].location = 1;//������shader�ж�Ӧ�ı���
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;//���ݵĸ�ʽ
		attributeDescriptions[1].offset = offsetof(Vertex, color);//������һ��vertexdata�е�ƫ��λ��

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;//������shader�ж�Ӧ�ı���
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;//���ݵĸ�ʽ
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);//������һ��vertexdata�е�ƫ��λ��

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

	//��CommandPools�е�Indices
	int GraphicCommandPoolIndex;
	int TransferCommandPoolIndex;
	int ComputeCommandPoolIndex;
	int PresentCommandPoolIndex;

	//��ActiveQueues�е�Indices
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

	//////////////////////////���Ի����Ļ�ͼ���ܣ��պ���ܷ�װ����������
	VkRenderPass renderPass;//�������ͼ���й�
	VkPipelineLayout pipelineLayout;//�������uniform�����й�
	VkDescriptorSetLayout descriptorSetLayout;//uniform���ݵĽṹ����һ��Pipeline������Щuniform����
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;//��uniform����Buffer��ImageView&Sampler��ɵĶ�����Ҫ��SetLayout�������Ľṹ���Ӧ
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