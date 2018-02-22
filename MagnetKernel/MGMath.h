#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/epsilon.hpp> 
#include <algorithm>
#include <vector>
#include <chrono>

namespace mgm {
	typedef glm::vec2 vec2;
	typedef glm::vec3 vec3;
	typedef glm::vec4 vec4;

	typedef glm::mat4 mat4;


	template <typename T>
	GLM_FUNC_QUALIFIER glm::tmat4x4<T, glm::defaultp> perspective(T fovy, T aspect, T zNear, T zFar) {
		glm::tmat4x4<T, glm::defaultp> Result = glm::perspective(fovy, aspect, zNear, zFar);
		Result[1][1] *= -static_cast<T>(1);
		return Result;
	}

	template <typename genType>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR genType radians(genType degrees)
	{
		return glm::radians(degrees);
	}

	template <typename T>
	T max(T a, T b)
	{
		return a > b ? a : b;
	}

	template <typename T>
	T min(T a, T b)
	{
		return b > a ? a : b;
	}

	template <typename T>
	std::vector<T> calculateNoneRepeatingArray(std::vector<T> rawArray)
	{
		std::vector<T> result;
		for (T rawindex : rawArray)
		{
			bool repeat = false;
			for (T resultindex : result)
			{
				if (resultindex == rawindex)
				{
					repeat = true;
					break;
				}
			}
			if (!repeat)
			{
				result.push_back(rawindex);
			}
		}
		return result;
	}
}