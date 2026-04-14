#pragma once

#include "qj_user_global.h"

class QApplication;

namespace CefInitializer {
QJ_USER_EXPORT void Init(QApplication* app, int argc, char* argv[]);
}
