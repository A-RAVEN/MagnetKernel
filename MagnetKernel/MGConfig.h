#pragma once
#include "MGRenderer.h"

namespace MGConfig
{
	static uint32_t UNIFORM_BIND_POINT_CAMERA = 0u;
	static uint32_t UNIFORM_BIND_POINT_MODEL_MATRIX = UNIFORM_BIND_POINT_CAMERA + sizeof(UniformBufferObject);
	static uint32_t UNIFORM_BIND_POINT_LIGHT = UNIFORM_BIND_POINT_MODEL_MATRIX + sizeof(mgm::mat4);
}