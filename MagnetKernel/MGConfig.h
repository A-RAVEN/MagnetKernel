#pragma once
#include "MGDebug.h"
#include "MGCommonStructs.h"
#include <map>
#include <string>

namespace MGConfig
{
	enum MGDataBindingType { DATA_BINDING_TYPE_COMBINED_IMAGE };
	const static uint32_t UNIFORM_BIND_POINT_CAMERA = 0u;
	const static uint32_t UNIFORM_BIND_POINT_MODEL_MATRIX = UNIFORM_BIND_POINT_CAMERA + sizeof(UniformViewBufferObject);
	const static uint32_t UNIFORM_BIND_POINT_LIGHT = UNIFORM_BIND_POINT_MODEL_MATRIX + sizeof(mgm::mat4);

	static std::string MG_SAMPLER_NORMAL = "mg_sampler_normal";
	
	void initConfig();

	MGSamplerInfo FindSamplerInfo(std::string info_name);
}