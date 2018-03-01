#pragma once
#include "Platform.h"
#include "MGImageLoad.h"
#include "MGMath.h"
#include <vector>

class MGRenderer;

//struct MG_Vertex {
//	mgm::vec3 pos;
//	mgm::vec3 color;
//	mgm::vec2 texCoord;
//
//	static VkVertexInputBindingDescription getBindingDescription() {
//		VkVertexInputBindingDescription bindingDescription = {};
//		bindingDescription.binding = 0;
//		bindingDescription.stride = sizeof(MG_Vertex);//一个vertexdata的大小
//		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
//		return bindingDescription;
//	}
//
//	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
//		std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {};
//		attributeDescriptions.resize(3);
//		attributeDescriptions[0].binding = 0;
//		attributeDescriptions[0].location = 0;
//		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
//		attributeDescriptions[0].offset = offsetof(MG_Vertex, pos);
//
//		attributeDescriptions[1].binding = 0;
//		attributeDescriptions[1].location = 1;//数据在shader中对应的变量
//		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;//数据的格式
//		attributeDescriptions[1].offset = offsetof(MG_Vertex, color);//数据在一个vertexdata中的偏移位置
//
//		attributeDescriptions[2].binding = 0;
//		attributeDescriptions[2].location = 2;//数据在shader中对应的变量
//		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;//数据的格式
//		attributeDescriptions[2].offset = offsetof(MG_Vertex, texCoord);//数据在一个vertexdata中的偏移位置
//
//		return attributeDescriptions;
//	}
//};
//
//struct MG_UniformBufferObject {
//	mgm::mat4 model;
//	mgm::mat4 view;
//	mgm::mat4 proj;
//};

//struct MGRenderPass
//{
//
//};

class MGModel;
class MGModelInstance;
class MGPipelineManager
{
public:
	MGPipelineManager();
	MGPipelineManager(MGRenderer* renderer);
	~MGPipelineManager();

	VkRenderPass renderPass;//与输出的图像有关

	void initPipeline();
	void releasePipeline();
	void cmdExecute(VkCommandBuffer commandBuffer,int frameBufferID);
	//void updatePipeline();
	MGModel* model;
	MGModelInstance* instance0;
	MGModelInstance* instance1;
private:

	MGRenderer* OwningRenderer;


	VkPipelineLayout pipelineLayout;//与输入的uniform数据有关
	VkDescriptorSetLayout descriptorSetLayout;//uniform数据的结构，即一个Pipeline中有哪些uniform输入
	VkDescriptorPool descriptorPool;
	//VkDescriptorSet descriptorSet;//由uniform数据Buffer和ImageView&Sampler组成的对象，需要与SetLayout所描述的结构相对应
	std::vector<VkDescriptorSet>descriptorSets;
	VkPipeline graphicsPipeline;

	Texture texture = {};
	Texture texture2 = {};
	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBufferMemory;
	mgm::vec3 light;
	//VkBuffer vertexBuffer;
	//VkDeviceMemory vertexBufferMemory;
	//VkBuffer indexBuffer;
	//VkDeviceMemory indexBufferMemory;

	void _prepareRenderpass();//深度缓冲在RenderPass中设置Attachment，同时在Pipeline中设置DepthStencilState,最后还要在Framebuffer中有对应的图片输出
	void _prepareDescriptorSetLayout();
	void _prepareGraphicPipeline();

	void _createDescriptorPool();//!
	void _allocateDescriptorSet(uint32_t SetCount, VkDescriptorSet* sets);//!

	void _prepareVerticesBuffer();
	void _prepareTextures(const char* file_path, Texture& tex);//!
	void _prepareUniformBuffer();
	void _writeDescriptorSet(VkDescriptorSet& set, Texture& tex, Texture& NorTex);//!

	VkShaderModule createShaderModule(const std::vector<char>& code);
};

