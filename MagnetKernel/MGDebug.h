#pragma once
#include "Platform.h"
#include <vector>

enum ExtensionType {
	EXTENSION_TYPE_INSTANCE,
	EXTENSION_TYPE_DEVICE
};

void MGCheckVKResultERR(VkResult result, const char* msg);

void MGThrowError(bool terminate,const char* errMessage);

void MGCheckExtensions(std::vector<const char*> extensions, ExtensionType type, VkPhysicalDevice device);

void MGCheckValidationLayerSupport(std::vector<const char*> validationLayers, ExtensionType type, VkPhysicalDevice device);

VkResult MGInstanceSetupDebugReportCallback(VkInstance instance);

void MGInstanceDestroyDebugReportCallback(VkInstance instance);

void ERRReport(const char* msg,bool shouldTerminate);