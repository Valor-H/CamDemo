#include "MockCamOptions.h"

namespace
{
MockCamOptions g_mockCamOptions;
}

MockCamOptions* CAMOptsPtr = &g_mockCamOptions;

std::string MockCamOptions::GetRecentFileList() const
{
    return "D:\\Temps\\新建 112.qjp;D:\\Temps\\12345 测试.qjp;D:\\Temps\\车削123.qjp";
}