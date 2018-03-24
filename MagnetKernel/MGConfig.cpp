#include "MGConfig.h"

namespace
{
	std::map<std::string, MGSamplerInfo> sampler_infos;
}

void MGConfig::initConfig()
{
	//初始化普通采样器信息
	MGSamplerInfo normal_sampler_info;
	normal_sampler_info.sampler_info = {};
	normal_sampler_info.sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	normal_sampler_info.sampler_info.magFilter = VK_FILTER_LINEAR;
	normal_sampler_info.sampler_info.minFilter = VK_FILTER_LINEAR;
	normal_sampler_info.sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	normal_sampler_info.sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	normal_sampler_info.sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	normal_sampler_info.sampler_info.anisotropyEnable = VK_TRUE;//当实际像素数量小于图片像素数量时，使用anistropic采样可以避免模糊
	normal_sampler_info.sampler_info.maxAnisotropy = 16;//采样点
	normal_sampler_info.sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;//超出范围的颜色
	normal_sampler_info.sampler_info.unnormalizedCoordinates = VK_FALSE;//为真时，映射坐标范围为[0,图片长宽]，为假时，映射坐标范围为[0,1]
	normal_sampler_info.sampler_info.compareEnable = VK_FALSE;
	normal_sampler_info.sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	normal_sampler_info.sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	normal_sampler_info.sampler_info.mipLodBias = 0.0f;
	normal_sampler_info.sampler_info.minLod = 0.0f;
	normal_sampler_info.sampler_info.maxLod = 0.0f;
	normal_sampler_info.sampler_name = MG_SAMPLER_NORMAL;

	sampler_infos[MG_SAMPLER_NORMAL] = normal_sampler_info;
	//sampler_infos.insert(std::pair<std::string, MGSamplerInfo>(MG_SAMPLER_NORMAL, normal_sampler_info));

}

MGSamplerInfo MGConfig::FindSamplerInfo(std::string info_name)
{
	std::map<std::string, MGSamplerInfo>::iterator l_it;
	l_it = sampler_infos.find(info_name);
	if (l_it != sampler_infos.end())
	{
		return l_it->second;
	}
	else
	{
		std::string err = "Sampler Information: " + info_name + " is not found!";
		ERRReport(err.c_str(), true);
	}
}