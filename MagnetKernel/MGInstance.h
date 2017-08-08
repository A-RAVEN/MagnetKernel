#pragma once
#include "Platform.h"
#include <vector>

struct MGDevice
{
	VkPhysicalDevice device;
	std::vector<VkQueueFamilyProperties> queueFamilies;
	VkQueueFlags fullUsage;
};

class MGInstance
{
public:
	MGInstance(std::string name);
	~MGInstance();
	void releaseInstance();
	VkInstance instance;

	std::vector<MGDevice> filterDevices(VkQueueFlags flags);

private:
	std::string projectName;
	std::vector<MGDevice> physicalDevices;

	void _createInstance();
	void _pickPhysicalDevices();
};
