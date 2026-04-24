#pragma once

#include <string>

class MockCamOptions
{
public:
    std::string GetRecentFileList() const;
};

extern MockCamOptions* CAMOptsPtr;