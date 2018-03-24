#pragma once
#include "Platform.h"
#include "MGDebug.h"
#include "MGMath.h"
#include "MGImageLoad.h"
#include "MGInstance.h"
#include "MGWindow.h"
#include "MGCommonStructs.h"
#include "MGConfig.h"
#include <vector>
#include <map>

class MGSwapChain;
class MGPipelineManager;

class MGRenderer
{
public:
	//uint32_t UNIFORM_BIND_POINT_CAMERA;
	//uint32_t UNIFORM_BIND_POINT_MODEL_MATRIX;
	//uint32_t UNIFORM_BIND_POINT_LIGHT;

	//Base Functions
	MGRenderer();
	MGRenderer(MGInstance* instance,MGWindow& window);
	~MGRenderer();
	void releaseRenderer();
	void updateUniforms();
	void renderFrame();
	void OnWindowResized();

	//Useful Member Value;
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	VkDevice LogicalDevice;
	MGDevice PhysicalDevice;

	MGSwapChain* SwapChain;

	MGPipelineManager* Pipeline;

	int GraphicQueueFamilyIndex;
	int TransferQueueFamilyIndex;
	int ComputeQueueFamilyIndex;
	int PresentQueueFamilyIndex;

	//MGSamplers TextureSamplers;
	std::map<std::string, VkSampler> Samplers;

	MGRendererSemaphores RendererSemaphores;

	//Help Functions
	uint32_t mgFindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	//未来可移动至贴图资源管理类
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& img, VkDeviceMemory& imageMemory);
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	VkImageView createImageView2D(VkImage image, VkFormat format, VkImageSubresourceRange subresourceRange);
	VkFormat findSupportedImageFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkCommandPool getCommandPool(MGUses use);
	VkQueue getQueue(MGUses use, int idealID);
	VkViewport createFullScreenViewport();
	VkRect2D createFullScreenRect();
	VkFence getPrimaryCmdBufferFence(int ID);

	VkSampler getTextureSampler(std::string sampler_name);
	//依据预设名获取采样器，如果找不到采样器会尝试自动创建
	void loadTextureSampler(std::string sampler_name = MGConfig::MG_SAMPLER_NORMAL);
	//依据预设名初始化采样器
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

	std::vector<VkFence> PrimaryCommandBufferFences = {};
	std::vector<bool> PrimaryFenceInited = {};
	std::vector<VkCommandBuffer> PrimaryCommandBuffers = {};

	void _selectPhysicalDevice(MGInstance* instance, VkSurfaceKHR windowSurface);
	void _initLogicalDevice();
	void _initCommandPools();
	void _deInitCommandPools();
	void _initPrimaryCommandBuffer();
	void _deinitRenderFences();
	void _recordPrimaryCommandBuffers();
	void _recordPrimaryCommandBuffer(uint8_t index);
	void _initSamplers();
	void _deInitSamplers();
	void _initSemaphores();
	void _deInitSemaphores();

	VkCommandBuffer beginSingleTimeCommands(VkCommandPool cmdPool);
	void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue submitqueue, VkCommandPool cmdPool, MGArrayStruct<VkSemaphore> waitSemaphores = { 0,nullptr }, MGArrayStruct<VkSemaphore> signalSemaphores = { 0,nullptr });


};

class MGSwapChain {
public:
	MGSwapChain(MGRenderer* renderer, MGWindow* window);
	~MGSwapChain();
	void releaseSwapChain();
	void initSwapChain();
	void createSwapchainFramebuffers(VkRenderPass renderPass,bool enableDepth);
	//初始化Swapchain的Framebuffer
	void destroySwapchainFramebuffers();

	VkFramebuffer getSwapchainFramebuffer(int index);
	uint32_t getSwapchainImageSize();

	VkSwapchainKHR Swapchain;

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