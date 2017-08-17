#include "Platform.h"
#include "MGWindow.h"
#include "MGRenderer.h"
#include <Windows.h>

#ifdef	GLFW_INCLUDE_VULKAN

void glfwResizeCallback(GLFWwindow* window, int w, int h) {
	MGWindow* mgwindow = reinterpret_cast<MGWindow*>(glfwGetWindowUserPointer(window));
	mgwindow->OnResize(w, h);
}

MGWindow::MGWindow(std::string title, int w, int h) {
	Width = w;
	Hight = h;
	surfaceValid = false;
	RelatingRenderer = nullptr;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	glfwwindow = glfwCreateWindow(Width, Hight, title.c_str(), nullptr, nullptr);
	glfwSetWindowUserPointer(glfwwindow, this);
	glfwSetWindowSizeCallback(glfwwindow, glfwResizeCallback);
}

MGWindow::~MGWindow() {}

void MGWindow::releaseWindow()
{
	glfwDestroyWindow(glfwwindow);
}

VkResult MGWindow::createWindowSurface(VkInstance instance) {

	VkResult result = glfwCreateWindowSurface(instance, glfwwindow, nullptr, &windowSurface);
	if (result == VK_SUCCESS) {
		surfaceValid = true;
	}
	return result;
}

bool MGWindow::getWindowSurface(VkSurfaceKHR* surface) {
	if (surfaceValid) {
		*surface = windowSurface;
	}
	return surfaceValid;
}

std::vector<const char*> MGWindow::getSurfaceExtensions()
{
	unsigned int glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> result;
	for (unsigned int i = 0; i < glfwExtensionCount; i++) {
		result.push_back(glfwExtensions[i]);
	}
	return result;
}

void MGWindow::UpdateWindow() {
	glfwPollEvents();
}

bool MGWindow::ShouldRun() {
	return !glfwWindowShouldClose(glfwwindow);
}

void MGWindow::OnResize(int w, int h) {
	Width = w;
	Hight = h;
	if (RelatingRenderer)
	{
		RelatingRenderer->OnWindowResized();
	}
	//std::cout << w << "\\" << h << std::endl;
	//RECT rect;
	//
	//if (GetWindowRect(glfwGetWin32Window(glfwwindow), &rect))
	//{
	//	int width = rect.right - rect.left;
	//	int height = rect.bottom - rect.top;
	//	std::cout << width << "/" << height << std::endl;
	//}
}

#endif
