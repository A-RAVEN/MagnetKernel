#pragma once
#include "BUILD_OPTIONS.h"

#if BUILD_USING_GLFW

#define GLFW_INCLUDE_VULKAN
//#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
//#include <GLFW/glfw3native.h>

#else
//Windows
#ifdef _WIN32

#define VK_USE_PLATFORM_WIN32_KHR 1
#include<Windows.h>


//Linux
#elif defined(_linux)

#define VK_USE_PLATFORM_XCB_KHR 1
#include<xcb/xcb.h>

//else
#else

#error Platform Not Support

#endif

#include<vulkan\vulkan.h>

#endif

