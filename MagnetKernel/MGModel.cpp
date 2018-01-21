#include "MGModel.h"
#include "MGShare.h"


MGModel::MGModel()
{
}


MGModel::~MGModel()
{
}

bool MGModel::loadOBJ(const std::string& path)
{
	mgReadFile(path);
	return false;
}
