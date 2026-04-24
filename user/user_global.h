#pragma once

#ifndef USER_GLOBAL_H
#define USER_GLOBAL_H

#include <QtCore/qglobal.h>


#ifdef user_EXPORTS
#define USER_EXPORT Q_DECL_EXPORT
#else
#define USER_EXPORT Q_DECL_IMPORT
#endif

#define QJ_NAMESPACE_FIT_USER_BEGIN namespace qianjizn { namespace user {
#define QJ_NAMESPACE_FIT_USER_END } }
#define QJ_USING_NAMESPACE_FIT_USER using namespace qianjizn::user;

#endif
