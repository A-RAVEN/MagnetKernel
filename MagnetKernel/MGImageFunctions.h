#pragma once
#include "Platform.h"
#include "MGDebug.h"

VkImageView mgCreateImageView2D(VkDevice device,VkImage image, VkFormat format, VkImageSubresourceRange subresourceRange) {
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange = subresourceRange;

	VkImageView imageView;
	MGCheckVKResultERR(vkCreateImageView(device, &viewInfo, nullptr, &imageView),"Failed to create ImageView");

	return imageView;
}

VkFormat mgFindSupportedImageFormat(VkPhysicalDevice physicalDevice,const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}
	throw std::runtime_error("failed to find supported format!");
}