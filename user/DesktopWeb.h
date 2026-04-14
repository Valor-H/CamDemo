#pragma once

#include "qj_user_global.h"

#include <QString>
#include <QUrl>
#include <QUrlQuery>

QJ_NAMESPACE_FIT_QJ_USER_BEGIN
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

inline QUrl buildPersonalProfileUrl(const QUrl& frontendBase, const QString& authToken)
{
    QUrl url = frontendBase.resolved(QUrl(QStringLiteral("profile-personal")));
    if (!authToken.isEmpty()) {
        QUrlQuery q;
        q.addQueryItem(QStringLiteral("token"), authToken);
        url.setQuery(q);
    }
    return url;
}

inline QUrl buildTeamUrl(const QUrl& frontendBase, const QString& authToken)
{
    QUrl url = frontendBase.resolved(QUrl(QStringLiteral("team")));
    if (!authToken.isEmpty()) {
        QUrlQuery q;
        q.addQueryItem(QStringLiteral("token"), authToken);
        url.setQuery(q);
    }
    return url;
}
}
QJ_NAMESPACE_FIT_QJ_USER_END
