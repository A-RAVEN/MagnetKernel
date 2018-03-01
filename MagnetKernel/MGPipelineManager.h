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
//		bindingDescription.stride = sizeof(MG_Vertex);//һ��vertexdata�Ĵ�С
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
//		attributeDescriptions[1].location = 1;//������shader�ж�Ӧ�ı���
//		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;//���ݵĸ�ʽ
//		attributeDescriptions[1].offset = offsetof(MG_Vertex, color);//������һ��vertexdata�е�ƫ��λ��
//
//		attributeDescriptions[2].binding = 0;
//		attributeDescriptions[2].location = 2;//������shader�ж�Ӧ�ı���
//		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;//���ݵĸ�ʽ
//		attributeDescriptions[2].offset = offsetof(MG_Vertex, texCoord);//������һ��vertexdata�е�ƫ��λ��
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

	VkRenderPass renderPass;//�������ͼ���й�

	void initPipeline();
	void releasePipeline();
	void cmdExecute(VkCommandBuffer commandBuffer,int frameBufferID);
	//void updatePipeline();
	MGModel* model;
	MGModelInstance* instance0;
	MGModelInstance* instance1;
private:

	MGRenderer* OwningRenderer;


	VkPipelineLayout pipelineLayout;//�������uniform�����й�
	VkDescriptorSetLayout descriptorSetLayout;//uniform���ݵĽṹ����һ��Pipeline������Щuniform����
	VkDescriptorPool descriptorPool;
	//VkDescriptorSet descriptorSet;//��uniform����Buffer��ImageView&Sampler��ɵĶ�����Ҫ��SetLayout�������Ľṹ���Ӧ
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

	void _prepareRenderpass();//��Ȼ�����RenderPass������Attachment��ͬʱ��Pipeline������DepthStencilState,���Ҫ��Framebuffer���ж�Ӧ��ͼƬ���
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

