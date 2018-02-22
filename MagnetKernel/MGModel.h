#pragma once
#include<string>
#include<vector>
#include"Platform.h"

class Vertex;
class MGRenderer;
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
class MGModel
{
public:
	MGModel(MGRenderer* renderer, uint8_t MaxBuffer = 2);
	~MGModel();
	bool loadOBJ(const std::string& path);
	void buildBuffers();
	void releaseBuffers();
	VkCommandBuffer GetCurrentCmdBuffer();
	std::vector<Vertex> VertexList;
	std::vector<uint32_t> IndexList;

	MGRenderer* OwningRenderer;

	void MGCmdDraw(VkCommandBuffer PcmdBuffer);

	void NotationRecordCmdBuffer(VkCommandBufferInheritanceInfo inheritanceInfo,VkFence WaitingFence);
	//Deprecated
private:
	CmdBuffers command_buffers;
	BufferStruct VertexBuffer;
	BufferStruct IndicesBuffer;
	void RecordCmdBuffer(VkCommandBufferInheritanceInfo inheritanceInfo);
	//Deprecated

};

