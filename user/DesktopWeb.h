#pragma once

#include <QString>
#include <QUrl>
#include <QUrlQuery>

/** 桌面端嵌入前端 URL（纯函数，便于单测与多宿主复用） */
namespace DesktopWeb
{
inline QString desktopClientQueryKey()
{
    return QStringLiteral("client");
}

inline QString desktopClientQueryValue()
{
    return QStringLiteral("desktop");
}

inline QUrl buildDesktopLoginUrl(const QUrl& frontendBase)
{
    QUrl url = frontendBase;
    if (!url.path().endsWith(QLatin1Char('/'))) {
        url.setPath(url.path() + QLatin1Char('/'));
    }
    QUrlQuery query(url);
    query.addQueryItem(desktopClientQueryKey(), desktopClientQueryValue());
    url.setQuery(query);
    return url;
}

/** 外置浏览器打开个人资料页（与前端 query 约定一致） */
inline QUrl buildPersonalProfileUrl(const QUrl& frontendBase, const QString& authToken)
{
    QUrl url = frontendBase.resolved(QUrl(QStringLiteral("personalProfile/personal")));
    if (!authToken.isEmpty()) {
        QUrlQuery q;
        q.addQueryItem(QStringLiteral("token"), authToken);
        url.setQuery(q);
    }
    return url;
}
} // namespace DesktopWeb
