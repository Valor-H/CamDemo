#include "UserSession.h"

namespace
{
const QString kTokenKey = QStringLiteral("token");
const QString kUserKey = QStringLiteral("user");
const QString kLoggedInKey = QStringLiteral("loggedIn");
} // namespace

UserSession::UserSession(QObject* parent)
    : QObject(parent)
{}

void UserSession::SetAuthenticatedState(bool on)
{
    if (_authenticated == on) {
        return;
    }
    _authenticated = on;
    emit AuthStateChanged(on);
}

void UserSession::ApplyFromLoginPayload(const QVariantMap& payload)
{
    const QString token = payload.value(kTokenKey).toString().trimmed();
    if (token.isEmpty()) {
        _authToken.clear();
        _currentUser.clear();
        SetAuthenticatedState(false);
        emit UserProfileChanged();
        return;
    }

    const QVariantMap user = payload.value(kUserKey).toMap();
    _authToken = token;
    if (!user.isEmpty()) {
        _currentUser = user;
    }
    SetAuthenticatedState(true);
    emit UserProfileChanged();
}

void UserSession::ApplyFromProbe(const QVariantMap& data)
{
    const bool loggedIn = data.value(kLoggedInKey).toBool();
    if (!loggedIn) {
        _authToken.clear();
        _currentUser.clear();
        SetAuthenticatedState(false);
        emit UserProfileChanged();
        return;
    }

    const QString token = data.value(kTokenKey).toString().trimmed();
    _authToken = token;
    // 启动恢复时只有 token，不再从缓存读用户信息
    // 用户信息由前端根据 token 重新从后端拉取并通过 ApplyFromLoginPayload 更新
    _currentUser.clear();
    SetAuthenticatedState(true);
    emit UserProfileChanged();
}

void UserSession::Logout()
{
    _authToken.clear();
    _currentUser.clear();
    SetAuthenticatedState(false);
    emit UserProfileChanged();
}
