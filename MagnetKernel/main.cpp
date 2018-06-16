//#define GLFW_INCLUDE_VULKAN
//#include <GLFW/glfw3.h>
#include <stdexcept>



#include "Platform.h"
#include "MGDebug.h"
#include "MGConfig.h"
#include "MGInstance.h"
#include "MGWindow.h"
#include "MGRenderer.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include "MGMath.h"

void test()
{
	float r = 0.4f;
	float alpha = r * r;
	float ndl = 0.01f;
	while (ndl < 1.0f)
	{
		float out = (alpha * alpha) / (3.1415926f * pow((ndl * ndl * (alpha * alpha - 1.0f) + 1.0f), 2.0));
		std::cout << ndl << "\t" << out << std::endl;
		ndl += 0.05f;
	}
}

int main() {
	try {
#ifdef BUILD_USING_GLFW
		glfwInit();
#endif
		//test();
		MGInstance instance("Magnet Kernel");
		MGConfig::initConfig();
		MGWindow window("Magnet Kernel", 800, 600);
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