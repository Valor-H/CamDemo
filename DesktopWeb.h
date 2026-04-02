#pragma once

#include <QString>
#include <QUrl>
#include <QUrlQuery>

/**
 * 桌面端（CamDemo）嵌入前端页面的 URL 构建工具。
 *
 * 设计目标：
 * - 单一职责：集中管理前端入口地址与 query 约定，避免多处硬编码
 * - 易测试：纯函数（输入 -> 输出），便于做单元测试
 * - 便于演进：后续若从 localhost 切到可配置，仅需修改此处
 */
namespace DesktopWeb
{
// 仅开发模式：固定本地 Vite dev server。
inline QUrl BuildDesktopBaseUrl()
{
    return QUrl(QStringLiteral("http://localhost:5173/"));
}

inline QString BuildDesktopLoginUrl()
{
    QUrl url = BuildDesktopBaseUrl();
    QUrlQuery query(url);
    query.addQueryItem(QStringLiteral("client"), QStringLiteral("desktop"));
    url.setQuery(query);
    return url.toString();
}

inline QString DesktopClientQueryKey()
{
    return QStringLiteral("client");
}

inline QString DesktopClientQueryValue()
{
    return QStringLiteral("desktop");
}

/** 外置浏览器打开个人资料页（与前端 query 约定一致，token 由 SPA 首屏消费后从地址栏移除） */
inline QUrl BuildPersonalProfileUrl(const QString& authToken)
{
    QUrl url = BuildDesktopBaseUrl().resolved(QUrl(QStringLiteral("personalProfile/personal")));
    if (!authToken.isEmpty()) {
        QUrlQuery q;
        q.addQueryItem(QStringLiteral("token"), authToken);
        url.setQuery(q);
    }
    return url;
}
} // namespace DesktopWeb

