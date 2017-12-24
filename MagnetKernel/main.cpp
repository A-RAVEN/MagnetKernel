//#define GLFW_INCLUDE_VULKAN
//#include <GLFW/glfw3.h>
#include <stdexcept>



#include "Platform.h"
#include "MGDebug.h"
#include "MGInstance.h"
#include "MGWindow.h"
#include "MGRenderer.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include "MGMath.h"


int main() {
	try {
#ifdef BUILD_USING_GLFW
		glfwInit();
#endif

		MGInstance instance("TEST Project");
		MGWindow window("test", 800, 600);
		MGCheckVKResultERR(window.createWindowSurface(instance.instance),"Failed to create Window Surface");
		VkSurfaceKHR surface;
		if (window.getWindowSurface(&surface)) {
			MGRenderer testRenderer(&instance, window);

			while (window.ShouldRun()) {
				window.UpdateWindow();
				testRenderer.updateUniforms();
				testRenderer.renderFrame();
			}
			testRenderer.releaseRenderer();		
		}

		window.releaseWindow();
		instance.releaseInstance();

#ifdef BUILD_USING_GLFW
		glfwTerminate();
#endif
	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;

}