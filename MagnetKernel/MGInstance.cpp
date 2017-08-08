#include "MGDebug.h"
#include "MGInstance.h"
#include "MGWindow.h"



MGInstance::MGInstance(std::string name)
{
	projectName = name;

	_createInstance();
	MGCheckVKResultERR(MGInstanceSetupDebugReportCallback(instance),"Failed to setup Instance Debug");
	_pickPhysicalDevices();

	//...
}


MGInstance::~MGInstance(){}

void MGInstance::releaseInstance()
{
	//...

	physicalDevices.clear();
	MGInstanceDestroyDebugReportCallback(instance);
	vkDestroyInstance(instance, nullptr);
}

void MGInstance::_createInstance()
{

	//application information
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = projectName.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "MagnetKernel";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	//Get Extensions that MGWindow requires
	std::vector<const char*>windowExtensions = MGWindow::getSurfaceExtensions();

	std::vector<const char*> extensions;
	for (unsigned int i = 0; i < windowExtensions.size(); i++) {
		extensions.push_back(windowExtensions[i]);
	}

#if BUILD_ENABLE_DEBUG//DEBUG
	extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif//DEBUG

	MGCheckExtensions(extensions, EXTENSION_TYPE_INSTANCE, VK_NULL_HANDLE);

	//instance creation information
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();

#if BUILD_ENABLE_DEBUG//DEBUG
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_LUNARG_standard_validation"
	};
	MGCheckValidationLayerSupport(validationLayers, EXTENSION_TYPE_INSTANCE, VK_NULL_HANDLE);
	createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	createInfo.ppEnabledLayerNames = validationLayers.data();
#else
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = VK_NULL_HANDLE;
#endif //DEBUG

	MGCheckVKResultERR(vkCreateInstance(&createInfo, nullptr, &instance),"Failed to create Instance");

}

void MGInstance::_pickPhysicalDevices() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		throw std::runtime_error("无法找到支持Vulkan的GPU!");
	}
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
	for (int i = 0; i < deviceCount; i++) {
		VkPhysicalDevice device = devices[i];
		MGDevice devicestruct;
		devicestruct.device = device;
		devicestruct.fullUsage = 0;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		devicestruct.queueFamilies.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, devicestruct.queueFamilies.data());
		
		for (auto properties : devicestruct.queueFamilies)
		{
			if (properties.queueCount > 0)
			{
				devicestruct.fullUsage |= properties.queueFlags;
			}
		}

		physicalDevices.push_back(devicestruct);
	}

	if (physicalDevices.size() == 0) {
		throw std::runtime_error("无法找到符合条件的GPU!");
	}
}

std::vector<MGDevice> MGInstance::filterDevices(VkQueueFlags flags)
{
	std::vector<MGDevice> devices;
	for (auto devicestruct : physicalDevices)
	{
		if (devicestruct.fullUsage & flags == flags)
		{
			devices.push_back(devicestruct);
		}
	}
	return devices;
}

