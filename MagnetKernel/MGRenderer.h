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
	//未来可移动至贴图资源管理类
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& img, VkDeviceMemory& imageMemory);
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

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

	//////////////////////////测试基本的绘图功能，日后可能封装到其它类中
	VkRenderPass renderPass;//与输出的图像有关
	VkPipelineLayout pipelineLayout;//与输入的uniform数据有关
	VkDescriptorSetLayout descriptorSetLayout;//uniform数据的结构，即一个Pipeline中有哪些uniform输入
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;//由uniform数据Buffer和ImageView&Sampler组成的对象，需要与SetLayout所描述的结构相对应
	VkPipeline graphicsPipeline;

	Texture texture = {};
	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBufferMemory;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	void _prepareRenderpass();//深度缓冲在RenderPass中设置Attachment，同时在Pipeline中设置DepthStencilState,最后还要在Framebuffer中有对应的图片输出
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
	//初始化Swapchain的Framebuffer
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