#include "MGDebug.h"
#include <iostream>
#include <sstream>

#ifdef _DEBUG

static VkDebugReportCallbackEXT debugReportCallBack;

//DEBUG的回调函数
VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanDebugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT obj_type,
	uint64_t src_obj,
	size_t location,
	int32_t msg_code,
	const char* layer_prefix,
	const char* msg,
	void* user_data
)
{
	std::ostringstream stream;
	stream << "VULKAN_DEBUG_";
	if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
	{
		stream << "INFO: ";
	}
	if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
	{
		stream << "WARNING: ";
	}
	if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
	{
		stream << "PERFORM_WARNING: ";
	}
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		stream << "ERROR: ";
	}
	if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
	{
		stream << "DEBUG: ";
	}
	//if (flags & VK_DEBUG_REPORT_FLAG_BITS_MAX_ENUM_EXT)
	//{
	//	stream << "FLAG_BITS_MAX_ENUM_EXT: ";
	//}
	stream << "(" << layer_prefix << ")";
	stream << msg << std::endl;

#ifdef _WIN32
	//if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
	//{
	//	MessageBox(NULL, stream.str().c_str(), "Vulkan Error!", 0);
	//}
	std::cout << stream.str().c_str() << std::endl;
#else
	std::cout << stream.str().c_str() << std::endl;
#endif

	return false;
}

void MGCheckVKResultERR(VkResult result,const char* msg)
{
	std::ostringstream stream;
	if (result < 0)
	{
		switch (result)
		{
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			stream << "VK_ERROR_OUT_OF_HOST_MEMORY" << std::endl;
			break;
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			stream << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << std::endl;
			break;
		case VK_ERROR_INITIALIZATION_FAILED:
			stream << "VK_ERROR_INITIALIZATION_FAILED" << std::endl;
			break;
		case VK_ERROR_DEVICE_LOST:
			stream << "VK_ERROR_DEVICE_LOST" << std::endl;
			break;
		case VK_ERROR_MEMORY_MAP_FAILED:
			stream << "VK_ERROR_MEMORY_MAP_FAILED" << std::endl;
			break;
		case VK_ERROR_LAYER_NOT_PRESENT:
			stream << "VK_ERROR_LAYER_NOT_PRESENT" << std::endl;
			break;
		case VK_ERROR_EXTENSION_NOT_PRESENT:
			stream << "VK_ERROR_EXTENSION_NOT_PRESENT" << std::endl;
			break;
		case VK_ERROR_FEATURE_NOT_PRESENT:
			stream << "VK_ERROR_FEATURE_NOT_PRESENT" << std::endl;
			break;
		case VK_ERROR_INCOMPATIBLE_DRIVER:
			stream << "VK_ERROR_INCOMPATIBLE_DRIVER" << std::endl;
			break;
		case VK_ERROR_TOO_MANY_OBJECTS:
			stream << "VK_ERROR_TOO_MANY_OBJECTS" << std::endl;
			break;
		case VK_ERROR_FORMAT_NOT_SUPPORTED:
			stream << "VK_ERROR_FORMAT_NOT_SUPPORTED" << std::endl;
			break;
		case VK_ERROR_SURFACE_LOST_KHR:
			stream << "VK_ERROR_SURFACE_LOST_KHR" << std::endl;
			break;
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
			stream << "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR" << std::endl;
			break;
		case VK_SUBOPTIMAL_KHR:
			stream << "VK_SUBOPTIMAL_KHR" << std::endl;
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
			stream << "VK_ERROR_OUT_OF_DATE_KHR" << std::endl;
			break;
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
			stream << "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR" << std::endl;
			break;
		case VK_ERROR_VALIDATION_FAILED_EXT:
			stream << "VK_ERROR_VALIDATION_FAILED_EXT" << std::endl;
			break;
		case VK_ERROR_INVALID_SHADER_NV:
			stream << "VK_ERROR_INVALID_SHADER_NV" << std::endl;
			break;
		default:
			stream << "VK_ERROR_UNKNOWN" << std::endl;
			break;
		}
		std::cout<<"Vulkan Result ERROR: "<< stream.str().c_str()<< "\n Message:" << msg << std::endl;
	}
}

void MGThrowError(bool terminate, const char* errMessage) {
	if (terminate) {
		throw std::runtime_error(errMessage);
	}
	else {
		std::cout << "Runtime Error !" << errMessage << std::endl;
	}
}

void MGCheckValidationLayerSupport(std::vector<const char*> validationLayers, ExtensionType type, VkPhysicalDevice device) {
	uint32_t layerCount;
	std::vector<VkLayerProperties> availableLayers;
	switch (type) {
	case EXTENSION_TYPE_INSTANCE:
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		availableLayers.resize(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
		break;
	case EXTENSION_TYPE_DEVICE:
		vkEnumerateDeviceLayerProperties(device, &layerCount, nullptr);
		availableLayers.resize(layerCount);
		vkEnumerateDeviceLayerProperties(device, &layerCount, availableLayers.data());
		break;
	default:
		break;
	}

	std::vector<const char*> unfoundLayers;

	for (const char* layerName : validationLayers)
	{
		bool layerFound = false;
		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}
		if (!layerFound)
		{
			unfoundLayers.push_back(layerName);
		}
	}
	if (unfoundLayers.size() > 0)
	{
		//debug report
		std::ostringstream stream;
		stream << "UNFOUND_LAYERS:" << std::endl;
		for (auto out : unfoundLayers)
		{
			stream << out << std::endl;
		}
		MGThrowError(false, stream.str().c_str());
	}
}

VkResult MGInstanceSetupDebugReportCallback(VkInstance instance) {
	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = VulkanDebugCallback;

	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr) {
		return func(instance, &createInfo, nullptr, &debugReportCallBack);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void ERRReport(const char* msg, bool shouldTerminate) {
	if (shouldTerminate) {
		throw std::runtime_error(msg);
	}
	else {
		std::cout << msg << std::endl;
	}
}

void MGInstanceDestroyDebugReportCallback(VkInstance instance) {
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) {
		func(instance, debugReportCallBack, nullptr);
	}
}

#else
void MGCheckVKResultERR(VkResult result, const char* msg){}
void MGCheckValidationLayerSupport(std::vector<const char*> validationLayers, ExtensionType type, VkPhysicalDevice device) {}
VkResult MGInstanceSetupDebugReportCallback(VkInstance instance) { return VK_SUCCESS; }
void ERRReport(const char* msg, bool shouldTerminate) {
	if (shouldTerminate) {
		throw std::runtime_error("msg");
	}
}
void MGInstanceDestroyDebugReportCallback(VkInstance instance) {}
void MGThrowError(bool terminate, const char* errMessage) {
	if (terminate) {
		throw std::runtime_error(errMessage);
	}
}
#endif

void MGCheckExtensions(std::vector<const char*> input, ExtensionType type, VkPhysicalDevice device)
{
	uint32_t extensionCount = 0;
	std::vector<VkExtensionProperties> extensions;
	switch (type) {
	case EXTENSION_TYPE_INSTANCE:
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		extensions.resize(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
		break;
	case EXTENSION_TYPE_DEVICE:
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		extensions.resize(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());
		break;
	default:
		break;
	}

	std::vector<const char*> unfoundExtensions;
	for (auto extensionname : input) {
		bool extensionFound = false;
		for (VkExtensionProperties i : extensions)
		{
			if (strcmp(i.extensionName, extensionname) == 0)
			{
				extensionFound = true;
				break;
			}
		}
		if (!extensionFound)
		{
			unfoundExtensions.push_back(extensionname);
		}
	}
	if (unfoundExtensions.size() > 0)
	{
		//debug report
		std::cout << "UNFOUND_EXTENSIONS:" << std::endl;
		for (auto out : unfoundExtensions)
		{
			std::cout << out << std::endl;
		}
	}
}



