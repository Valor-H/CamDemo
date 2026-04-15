#pragma once

#ifndef QJ_USER_GLOBAL_H
#define QJ_USER_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(QJ_USER_LIBRARY)
#define QJ_USER_EXPORT Q_DECL_EXPORT
#else
#define QJ_USER_EXPORT Q_DECL_IMPORT
#endif

#define QJ_NAMESPACE_FIT_QJ_USER_BEGIN namespace qianjizn { namespace qj_user {
#define QJ_NAMESPACE_FIT_QJ_USER_END } }
#define QJ_USING_NAMESPACE_FIT_QJ_USER using namespace qianjizn::qj_user;

#endif
