#pragma once
#include "Platform.h"
#include <string>
#include<iostream>
#include<vector>

struct MGSurfaceImageSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;//Basic surface capabilities (min/max number of images in swap chain, min/max width and height of images)
	std::vector<VkSurfaceFormatKHR> formats;//Surface formats (pixel format, color space)
	std::vector<VkPresentModeKHR> presentModes;//Available presentation modes
};

class MGWindow {
public:

#if VK_USE_PLATFORM_WIN32_KHR
	bool window_should_run = true;
#endif

	MGWindow(std::string title, int w, int h);
	~MGWindow();
	void releaseWindow();

	void UpdateWindow();

	bool ShouldRun();

	void OnResize(int w, int h);

	VkResult createWindowSurface(VkInstance instance);
	bool getWindowSurface(VkSurfaceKHR* surface);

	MGSurfaceImageSupportDetails querySurfaceImageSupport(VkPhysicalDevice device);

	static std::vector<const char*> getSurfaceExtensions();

	int Width = 600;
	int Hight = 600;

private:


	VkSurfaceKHR windowSurface;
	bool surfaceValid = false;
#ifdef	GLFW_INCLUDE_VULKAN
	GLFWwindow* glfwwindow;
#endif
#if VK_USE_PLATFORM_WIN32_KHR
	HINSTANCE _win32_instance = NULL;
	HWND _win32_window = NULL;
	std::string _win32_class_name;
	static uint64_t _win32_class_id_counter;
#endif
};