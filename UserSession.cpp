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

void UserSession::setAuthenticatedState(bool on)
{
    if (_authenticated == on) {
        return;
    }
    _authenticated = on;
    emit authStateChanged(on);
}

void UserSession::applyFromLoginPayload(const QVariantMap& payload)
{
    const QString token = payload.value(kTokenKey).toString().trimmed();
    if (token.isEmpty()) {
        _authToken.clear();
        _currentUser.clear();
        setAuthenticatedState(false);
        emit userProfileChanged();
        return;
    }

    const QVariantMap user = payload.value(kUserKey).toMap();
    _authToken = token;
    if (!user.isEmpty()) {
        _currentUser = user;
    }
    setAuthenticatedState(true);
    emit userProfileChanged();
}

void UserSession::applyFromProbe(const QVariantMap& data)
{
    const bool loggedIn = data.value(kLoggedInKey).toBool();
    if (!loggedIn) {
        _authToken.clear();
        _currentUser.clear();
        setAuthenticatedState(false);
        emit userProfileChanged();
        return;
    }

    const QString token = data.value(kTokenKey).toString().trimmed();
    _authToken = token;
    // 启动恢复时只有 token，不再从缓存读用户信息
    // 用户信息由前端根据 token 重新从后端拉取并通过 applyFromLoginPayload 更新
    _currentUser.clear();
    setAuthenticatedState(true);
    emit userProfileChanged();
}

void UserSession::logout()
{
    _authToken.clear();
    _currentUser.clear();
    setAuthenticatedState(false);
    emit userProfileChanged();
}
