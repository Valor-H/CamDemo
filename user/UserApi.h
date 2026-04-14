#pragma once

#include <QtCore/qglobal.h>

#if defined(USER_LIBRARY_BUILD)
#  define USER_API Q_DECL_EXPORT
#elif defined(USER_LIBRARY_SHARED)
#  define USER_API Q_DECL_IMPORT
#else
#  define USER_API
#endif
