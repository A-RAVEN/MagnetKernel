#pragma once
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include <stb_image.h>
#include "Platform.h"
#include "MGDebug.h"

struct Texture {
	VkImage image;
	VkDeviceMemory imageMemory;
	VkImageView imageView;
	VkSampler sampler;
};

struct MGRawImage {
	stbi_uc* pixels;
	int texWidth, texHeight, texChannels;
	VkDeviceSize getImageSize() {
		return texWidth * texHeight * 4;
	}
};

static MGRawImage mgLoadRawImage(const char* filePath)
{
	MGRawImage result;
	result.pixels = stbi_load(filePath, &result.texWidth, &result.texHeight, &result.texChannels, STBI_rgb_alpha);
	return result;
}