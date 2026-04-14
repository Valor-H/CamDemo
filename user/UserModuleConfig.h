#pragma once

#include "qj_user_global.h"

#include <QString>
#include <QUrl>

QJ_NAMESPACE_FIT_QJ_USER_BEGIN

struct UserModuleConfig
{
    QUrl apiBaseUrl { QStringLiteral("http://localhost:8080/") };
    QUrl frontendBaseUrl { QStringLiteral("http://localhost:5173/") };
    QString settingsOrg { QStringLiteral("QianJiZN") };
    QString settingsApp { QStringLiteral("CamDemo") };
    QString authTokenKey { QStringLiteral("auth/token") };
};

QJ_NAMESPACE_FIT_QJ_USER_END
