#include "MGModel.h"
#include "MGShare.h"
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include "MGMath.h"
#include "MGRenderer.h"


MGModel::MGModel(MGRenderer* renderer, uint8_t MaxBuffer)
{
	OwningRenderer = renderer;
}


MGModel::~MGModel()
{
}

void addVertex(std::string key, std::map<std::string, uint32_t >* search_map, std::vector<Vertex>* vertexList, std::vector<uint32_t>* indexList, Vertex* newVertex)
{
	//std::map<std::string, uint16_t >::iterator l_it;
	//l_it = search_map.find(key);
	if ((*search_map)[key])
	{
		uint32_t index = (*search_map)[key];
		indexList->push_back(index);
	}
	else
	{
		indexList->push_back(vertexList->size());
		(*search_map)[key] = vertexList->size();
		vertexList->push_back(*newVertex);
	}
}

bool MGModel::loadOBJ(const std::string& path)
{
	std::ifstream inf;
	inf.open(path,std::ios_base::in);
	if (inf.good())
	{
		std::vector<mgm::vec3> vGroup;
		std::vector<mgm::vec3> vnGroup;
		std::vector<mgm::vec2> vtGroup;
		std::map<std::string, uint32_t>indicesMap;
		std::string line;
		bool attributes_state[3] = { false };
		while (std::getline(inf,line))
		{
			if (line.length() < 2)
				continue;
			if (line[0] == 'v')
			{
				std::istringstream s(line);
				std::string head;
				if (line[1] == 't')
				{
					//顶点UV
					mgm::vec2 newVT(0.0f, 0.0f);
					s >> head >> newVT.s >> newVT.t;
					newVT.t = 1.0f - newVT.t;
					vtGroup.push_back(newVT);
					attributes_state[1] = true;
				}
				else if (line[1] == 'n')
				{
					//顶点法线
					mgm::vec3 newVN(0.0f, 0.0f, 0.0f);
					s >> head >> newVN.x >> newVN.y >> newVN.z;
					vnGroup.push_back(newVN);
					attributes_state[2] = true;
				}
				else
				{
					//顶点	
					mgm::vec3 newV(0.0f, 0.0f, 0.0f);
					s >> head >> newV.x >> newV.y >> newV.z;
					vGroup.push_back(newV);
					attributes_state[0] = true;
				}
			}
			else if (line[0] == 'f')
			{
				//面
				int face_type = 0;
				line.erase(line.find_last_not_of(" ") + 1);
				for (uint32_t k = line.size() - 1; k > 0; k--)
				{
					if (line[k] == ' ')
						face_type++;
				}
				if (face_type == 3)
				{
					std::string face_string;
					std::istringstream s(line);
					s >> face_string;
					for (uint8_t i = 0; i < 3; i++)
					{
						s >> face_string;
						std::string fs = face_string;
						for (uint32_t k = fs.size() - 1; k > 0; k--)
						{
							if (fs[k] == '/')
							{
								fs[k] = ' ';
							}
						}
						std::istringstream fss(fs);
						uint32_t index;
						Vertex newVertex;
						if (attributes_state[0])
						{
							fss >> index;
							index--;
							newVertex.pos = vGroup[index];
						}
						if (attributes_state[1])
						{
							fss >> index;
							index--;
							newVertex.texCoord = vtGroup[index];
						}
						if (attributes_state[2])
						{
							fss >> index;
							index--;
							newVertex.color = vnGroup[index];
						}
						addVertex(face_string,&indicesMap,&VertexList, &IndexList, &newVertex);
					}
				}
				if (face_type == 4)
				{
					std::string face_strings[4];
					Vertex verices[4];
					std::istringstream s(line);
					s >> face_strings[0];
					for (uint8_t i = 0; i < 4; i++)
					{
						s >> face_strings[i];
						std::string fs = face_strings[i];
						for (uint32_t k = fs.size() - 1; k > 0; k--)
						{
							if (fs[k] == '/')
							{
								fs[k] = ' ';
							}
						}
						std::istringstream fss(fs);
						uint32_t index;
						if (attributes_state[0])
						{
							fss >> index;
							index--;
							verices[i].pos = vGroup[index];
						}
						if (attributes_state[1])
						{
							fss >> index;
							index--;
							verices[i].texCoord = vtGroup[index];
						}
						if (attributes_state[2])
						{
							fss >> index;
							index--;
							verices[i].color = vnGroup[index];
						}
					}
					addVertex(face_strings[0], &indicesMap, &VertexList, &IndexList, &verices[0]);
					addVertex(face_strings[1], &indicesMap, &VertexList, &IndexList, &verices[1]);
					addVertex(face_strings[2], &indicesMap, &VertexList, &IndexList, &verices[2]);
					uint32_t zeroIndex = IndexList.size() - 3;
					IndexList.push_back(IndexList[zeroIndex + 2]);
					addVertex(face_strings[3], &indicesMap, &VertexList, &IndexList, &verices[3]);
					IndexList.push_back(IndexList[zeroIndex]);
				}
			}
		}
		inf.close();
		return true;
	}
	return false;
}

void MGModel::MGCmdDraw(VkCommandBuffer PcmdBuffer)
{
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(PcmdBuffer, 0, 1, &VertexBuffer.Buffer, offsets);
	vkCmdBindIndexBuffer(PcmdBuffer, IndicesBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);///////!!!!!!!!!!!ATTENTION:VK_INDEX_TYPE
	vkCmdDrawIndexed(PcmdBuffer, static_cast<uint32_t>(IndexList.size()), 1, 0, 0, 0);
}

void MGModel::buildBuffers()
{
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VkDeviceSize bufferSize = sizeof(VertexList[0]) * VertexList.size();
	OwningRenderer->createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VertexBuffer.Buffer, VertexBuffer.BufferMemory);

	void* data;
	vkMapMemory(OwningRenderer->LogicalDevice, VertexBuffer.BufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, VertexList.data(), (size_t)bufferSize);
	vkUnmapMemory(OwningRenderer->LogicalDevice, VertexBuffer.BufferMemory);

	//OwningRenderer->createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

	//OwningRenderer->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	//vkDestroyBuffer(OwningRenderer->LogicalDevice, stagingBuffer, nullptr);
	//vkFreeMemory(OwningRenderer->LogicalDevice, stagingBufferMemory, nullptr);
	////////////////
	///////////////
	bufferSize = sizeof(IndexList[0]) * IndexList.size();

	//VkBuffer stagingBuffer;
	//VkDeviceMemory stagingBufferMemory;
	OwningRenderer->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data2;
	vkMapMemory(OwningRenderer->LogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data2);
	memcpy(data2, IndexList.data(), (size_t)bufferSize);
	vkUnmapMemory(OwningRenderer->LogicalDevice, stagingBufferMemory);

	OwningRenderer->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, IndicesBuffer.Buffer, IndicesBuffer.BufferMemory);

	OwningRenderer->copyBuffer(stagingBuffer, IndicesBuffer.Buffer, bufferSize);

	vkDestroyBuffer(OwningRenderer->LogicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(OwningRenderer->LogicalDevice, stagingBufferMemory, nullptr);
}

void MGModel::releaseBuffers()
{
	vkDestroyBuffer(OwningRenderer->LogicalDevice, VertexBuffer.Buffer, nullptr);
	vkFreeMemory(OwningRenderer->LogicalDevice, VertexBuffer.BufferMemory, nullptr);
	vkDestroyBuffer(OwningRenderer->LogicalDevice, IndicesBuffer.Buffer, nullptr);
	vkFreeMemory(OwningRenderer->LogicalDevice, IndicesBuffer.BufferMemory, nullptr);
}
