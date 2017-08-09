#pragma once
#include <vector>
#include <fstream>

static std::vector<char> mgReadFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);//ate:读取位置位于文本末尾，这样可以直接通过tellg获取文本长度

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}