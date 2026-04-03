#pragma once

#include <QUrl>
#include <QVariantMap>

/** 登录 Web 页：路径解析、载荷清洗、与 DesktopWeb 一致的来源校验（可单测） */
namespace LoginWebAuth
{
enum class AuthRoute
{
    Login,
    Register,
    Reset,
    Unknown
};

QString ExtractAuthRoutePath(const QUrl& url);
AuthRoute RouteFromPath(const QString& path);
QVariantMap SanitizeLoginPayload(const QVariantMap& input);

bool IsTrustedUiSource(const QUrl& currentUrl, const QUrl& loginPageUrl);
bool IsTrustedInvokeSource(const QUrl& currentUrl, const QUrl& loginPageUrl);
} // namespace LoginWebAuth
