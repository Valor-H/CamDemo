#pragma once

#include "cloud_server_global.h"

#include <QString>

class QCefView;
class QJsonObject;

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN
class UserAuthService;
/** 构建桌面运行时注入数据。 */
QJsonObject BuildDesktopRuntimePayload(const UserAuthService* authService);
/** 构建桌面运行时注入脚本。 */
QString BuildDesktopRuntimeInjectionScript(const UserAuthService* authService);
/** 向指定浏览器视图注入桌面运行时。 */
bool InjectDesktopRuntimeIntoView(QCefView* view, const UserAuthService* authService, const QString& sourceUrl = QString());

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
