#pragma once
#include <string>
#include <vector>
#include "Platform.h"
#include "MGCommonStructs.h"

//struct Vertex;
class MGRenderer;

class MGModel
{
public:
	MGModel(MGRenderer* renderer, uint8_t MaxBuffer = 2);
	~MGModel();
	bool loadOBJ(const std::string& path);
	void buildBuffers();
	void releaseBuffers();
	//VkCommandBuffer GetCurrentCmdBuffer();
	std::vector<Vertex> VertexList;
	std::vector<uint32_t> IndexList;

	MGRenderer* OwningRenderer;

	void MGCmdDraw(VkCommandBuffer PcmdBuffer);

	//void NotationRecordCmdBuffer(VkCommandBufferInheritanceInfo inheritanceInfo,VkFence WaitingFence);
	//Deprecated
private:
	//CmdBuffers command_buffers;
	BufferStruct VertexBuffer;
	BufferStruct IndicesBuffer;
	//void RecordCmdBuffer(VkCommandBufferInheritanceInfo inheritanceInfo);
	//Deprecated

};

