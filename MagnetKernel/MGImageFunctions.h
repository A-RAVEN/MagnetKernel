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