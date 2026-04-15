#pragma once

#include "user_global.h"

#include <QObject>
#include <QString>
#include <QVariantMap>

#ifdef Q_MOC_RUN
namespace qianjizn { namespace user {
#else
QJ_NAMESPACE_FIT_USER_BEGIN
#endif

class USER_EXPORT UserSession final : public QObject
{
    Q_OBJECT

public:
    explicit UserSession(QObject* parent = nullptr);

    bool IsAuthenticated() const { return _authenticated; }
    QString AuthToken() const { return _authToken; }
    QVariantMap CurrentUser() const { return _currentUser; }

    void ApplyFromLoginPayload(const QVariantMap& payload);
    void ApplyFromProbe(const QVariantMap& data);
    void Logout();

signals:
    void AuthStateChanged(bool authenticated);
    void UserProfileChanged();

private:
    void SetAuthenticatedState(bool on);

    bool _authenticated { false };
    QString _authToken;
    QVariantMap _currentUser;
};

#ifdef Q_MOC_RUN
} }
#else
QJ_NAMESPACE_FIT_USER_END
#endif
