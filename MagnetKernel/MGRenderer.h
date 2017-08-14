#pragma once
#include "Platform.h"
#include "MGMath.h"
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

struct UniformBufferObject {
	mgm::mat4 model;
	mgm::mat4 view;
	mgm::mat4 proj;
};

struct Texture {
	VkImage image;
	VkDeviceMemory imageMemory;
	VkImageView imageView;
	VkSampler sampler;
};

struct MGSamplers {
	VkSampler MG_SAMPLER_NORMAL;
};

struct MGRendererSemaphores {
	VkSemaphore MG_SEMAPHORE_SWAPCHAIN_IMAGE_AVAILABLE;
	VkSemaphore MG_SEMAPHORE_RENDER_FINISH;
	void initSemaphores(VkDevice logicalDevice) {
		VkSemaphoreCreateInfo createinfo = {};
		createinfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		MGCheckVKResultERR(vkCreateSemaphore(logicalDevice, &createinfo, nullptr, &MG_SEMAPHORE_SWAPCHAIN_IMAGE_AVAILABLE),"MG_SEMAPHORE_SWAPCHAIN_IMAGE_AVAILABLE Create Failed!");
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

class MGRenderer
{
public:
	//Base Functions
	MGRenderer();
	MGRenderer(MGInstance* instance,MGWindow& window);
	~MGRenderer();
	void releaseRenderer();
	void updateUniforms();
	void renderFrame();

	//Useful Member Value;
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	VkDevice LogicalDevice;
	MGDevice PhysicalDevice;

	int GraphicQueueFamilyIndex;
	int TransferQueueFamilyIndex;
	int ComputeQueueFamilyIndex;
	int PresentQueueFamilyIndex;

	MGSamplers TextureSamplers;

	MGRendererSemaphores RendererSemaphores;

	//Help Functions
	uint32_t mgFindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	//δ�����ƶ�����ͼ��Դ������
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& img, VkDeviceMemory& imageMemory);
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

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

	std::vector<VkCommandBuffer> PrimaryCommandBuffers = {};

	void _selectPhysicalDevice(MGInstance* instance, VkSurfaceKHR windowSurface);
	void _initLogicalDevice();
	void _initCommandPools();
	void _deInitCommandPools();
	void _initPrimaryCommandBuffer();
	void _initSamplers();
	void _deInitSamplers();
	void _initSemaphores();
	void _deInitSemaphores();

	//////////////////////////���Ի����Ļ�ͼ���ܣ��պ���ܷ�װ����������
	VkRenderPass renderPass;//�������ͼ���й�
	VkPipelineLayout pipelineLayout;//�������uniform�����й�
	VkDescriptorSetLayout descriptorSetLayout;//uniform���ݵĽṹ����һ��Pipeline������Щuniform����
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;//��uniform����Buffer��ImageView&Sampler��ɵĶ�����Ҫ��SetLayout�������Ľṹ���Ӧ
	VkPipeline graphicsPipeline;

	Texture texture = {};
	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBufferMemory;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	void _prepareRenderpass();//��Ȼ�����RenderPass������Attachment��ͬʱ��Pipeline������DepthStencilState,���Ҫ��Framebuffer���ж�Ӧ��ͼƬ���
	void _prepareDescriptorSetLayout();
	void _prepareGraphicPipeline();

	void _createDescriptorPool();//!
	void _allocateDescriptorSet();//!

	void _prepareVerticesBuffer();
	void _prepareTextures();//!
	void _prepareUniformBuffer();
	void _writeDescriptorSet();//!

	VkShaderModule createShaderModule(const std::vector<char>& code);
	///////////////////////////

	VkCommandBuffer beginSingleTimeCommands(VkCommandPool cmdPool);
	void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue submitqueue, VkCommandPool cmdPool);


};

class MGSwapChain {
public:
	MGSwapChain(MGRenderer* renderer, MGWindow* window);
	~MGSwapChain();
	void releaseSwapChain();
	
	void createSwapchainFramebuffers(VkRenderPass renderPass,bool enableDepth);
	//��ʼ��Swapchain��Framebuffer
	void destroySwapchainFramebuffers();

	VkFramebuffer getSwapchainFramebuffer(int index);
	int getSwapchainImageSize();

	VkSwapchainKHR swapchain;

	VkFormat SwapchainImageFormat;
	VkExtent2D SwapchainExtent;

private:
	MGRenderer* OwningRenderer;
	MGWindow* RelatingWindow;

	std::vector<VkImage>SwapchainImages;
	std::vector<VkImageView> SwapchainImageViews;
	std::vector<VkFramebuffer> SwapchainFramebuffers;

	Texture DepthTexture;

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities, int actualWidth, int actualHeight);
};