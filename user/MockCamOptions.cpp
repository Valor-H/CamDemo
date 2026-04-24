#include "MockCamOptions.h"

namespace
{
MockCamOptions g_mockCamOptions;
}

MockCamOptions* CAMOptsPtr = &g_mockCamOptions;

std::string MockCamOptions::GetRecentFileList() const
{
    return "D:\\TEST_DOC\\新建 112.qjp;D:\\TEST_DOC\\12345 测试.qjp;D:\\TEST_DOC\\车削123.qjp";
}