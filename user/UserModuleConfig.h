#pragma once

#include <QString>
#include <QUrl>

/** 用户模块运行时配置（宿主可覆盖 org/app 与端点） */
struct UserModuleConfig
{
    QUrl apiBaseUrl { QStringLiteral("http://localhost:8080/") };
    QUrl frontendBaseUrl { QStringLiteral("http://localhost:5173/") };
    QString settingsOrg { QStringLiteral("QianJiZN") };
    QString settingsApp { QStringLiteral("CamDemo") };
    QString authTokenKey { QStringLiteral("auth/token") };
};
