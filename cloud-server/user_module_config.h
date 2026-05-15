#pragma once

#include "cloud_server_global.h"

#include <QString>
#include <QUrl>

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

struct CLOUD_SERVER_EXPORT UserModuleConfig
{
    QUrl apiBaseUrl { QStringLiteral("http://localhost:8080") };
    QUrl websocketUrl { QStringLiteral("ws://localhost:8091/ws") };
    QUrl helpDocUrl { QStringLiteral("http://localhost:4173") };
    QUrl frontendBaseUrl { QStringLiteral("http://localhost:31870/") };
    QUrl externalFrontendBaseUrl { QStringLiteral("http://localhost:5173/") };
    QUrl mockServiceUrl { QStringLiteral("http://localhost:3001") };
    QString settingsOrg { QStringLiteral("QianJiZN") };
    QString settingsApp { QStringLiteral("QJCAM") };
    QString authTokenKey { QStringLiteral("auth/token") };
};

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
