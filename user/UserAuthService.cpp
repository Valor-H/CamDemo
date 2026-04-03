#include "UserAuthService.h"

#include "AuthHttpClient.h"
#include "LoginDialog.h"
#include "DesktopWeb.h"

#include <QSettings>
#include <QWidget>

namespace
{
constexpr int kWindowActivateRefreshDebounceMs = 800;
constexpr int kWindowActivateRefreshThrottleMs = 12000;

QString apiBaseStringForClient(const QUrl& u)
{
    QString s = u.toString(QUrl::RemoveQuery | QUrl::RemoveFragment);
    while (s.endsWith(QLatin1Char('/'))) {
        s.chop(1);
    }
    return s;
}
} // namespace

UserAuthService::UserAuthService(const UserModuleConfig& cfg, QObject* parent)
    : QObject(parent)
    , _cfg(cfg)
    , _userSession(this)
    , _authClient(new AuthHttpClient(apiBaseStringForClient(_cfg.apiBaseUrl), this))
{
    _windowActivateRefreshDebounceTimer = new QTimer(this);
    _windowActivateRefreshDebounceTimer->setSingleShot(true);
    connect(_windowActivateRefreshDebounceTimer,
            &QTimer::timeout,
            this,
            &UserAuthService::tryRefreshUserProfileOnWindowActivate);
}

UserAuthService::~UserAuthService()
{
    cancelAllPendingRequests();
}

void UserAuthService::cancelAllPendingRequests()
{
    if (_authClient) {
        _authClient->cancelAll();
    }
}

void UserAuthService::showLoginDialog(QWidget* parent)
{
    const QUrl loginUrl = DesktopWeb::buildDesktopLoginUrl(_cfg.frontendBaseUrl);
    LoginDialog dlg(parent, loginUrl);
    connect(&dlg, &LoginDialog::loginSucceeded, this, &UserAuthService::onLoginSucceeded);
    dlg.exec();
}

void UserAuthService::logout()
{
    cancelAllPendingRequests();
    _userHydrationInFlight = false;
    clearAuthTokenFromSettings();
    _userSession.logout();
}

void UserAuthService::onLoginSucceeded(const QVariantMap& payload)
{
    cancelAllPendingRequests();
    saveAuthTokenToSettings(payload.value(QStringLiteral("token")).toString());
    _userSession.applyFromLoginPayload(payload);
}

void UserAuthService::initFromStoredToken()
{
    const QString token = loadAuthTokenFromSettings();
    if (token.isEmpty()) {
        return;
    }

    QVariantMap data;
    data.insert(QStringLiteral("token"), token);
    data.insert(QStringLiteral("loggedIn"), true);
    _userSession.applyFromProbe(data);
    startDirectUserHydration(token, true);
}

void UserAuthService::startDirectUserHydration(const QString& token, bool allowRefresh)
{
    const QString trimmed = token.trimmed();
    if (trimmed.isEmpty()) {
        _userHydrationInFlight = false;
        return;
    }

    cancelAllPendingRequests();
    _userHydrationInFlight = true;
    fetchCurrentUserDirect(trimmed, allowRefresh);
}

void UserAuthService::fetchCurrentUserDirect(const QString& token, bool allowRefresh)
{
    _authClient->post(QStringLiteral("/api/user/current"), token, 10,
        [this, token, allowRefresh](const AuthHttpClient::Response& resp) {
            if (!resp.networkOk) {
                _userHydrationInFlight = false;
                return;
            }

            if (resp.bizCode == 200) {
                const QVariantMap userMap = resp.data.value(QStringLiteral("user")).toMap();
                if (userMap.isEmpty()) {
                    _userHydrationInFlight = false;
                    return;
                }

                QVariantMap payload;
                payload.insert(QStringLiteral("token"), token);
                payload.insert(QStringLiteral("user"), userMap);
                _userSession.applyFromLoginPayload(payload);
                _userHydrationInFlight = false;
                return;
            }

            if (resp.bizCode == 401 && allowRefresh) {
                refreshTokenDirectAndRetry(token);
                return;
            }

            if (resp.bizCode == 401) {
                cancelAllPendingRequests();
                clearAuthTokenFromSettings();
                _userSession.logout();
                _userHydrationInFlight = false;
                return;
            }

            _userHydrationInFlight = false;
        });
}

void UserAuthService::refreshTokenDirectAndRetry(const QString& token)
{
    _authClient->post(QStringLiteral("/api/auth/refresh"), token, 10,
        [this, token](const AuthHttpClient::Response& resp) {
            if (!resp.networkOk) {
                _userHydrationInFlight = false;
                return;
            }

            if (resp.bizCode == 200) {
                fetchCurrentUserDirect(token, false);
                return;
            }

            if (resp.bizCode == 401) {
                cancelAllPendingRequests();
                clearAuthTokenFromSettings();
                _userSession.logout();
                _userHydrationInFlight = false;
                return;
            }

            _userHydrationInFlight = false;
        });
}

void UserAuthService::saveAuthTokenToSettings(const QString& token)
{
    QSettings settings(_cfg.settingsOrg, _cfg.settingsApp);
    const QString trimmed = token.trimmed();
    if (trimmed.isEmpty()) {
        settings.remove(_cfg.authTokenKey);
    } else {
        settings.setValue(_cfg.authTokenKey, trimmed);
    }
    settings.sync();
}

QString UserAuthService::loadAuthTokenFromSettings() const
{
    QSettings settings(_cfg.settingsOrg, _cfg.settingsApp);
    return settings.value(_cfg.authTokenKey).toString().trimmed();
}

void UserAuthService::clearAuthTokenFromSettings()
{
    QSettings settings(_cfg.settingsOrg, _cfg.settingsApp);
    settings.remove(_cfg.authTokenKey);
    settings.sync();
}

void UserAuthService::onWindowActivateEvent()
{
    scheduleWindowActivateRefresh();
}

void UserAuthService::scheduleWindowActivateRefresh()
{
    if (!_windowActivateRefreshDebounceTimer) {
        return;
    }
    _windowActivateRefreshDebounceTimer->start(kWindowActivateRefreshDebounceMs);
}

void UserAuthService::tryRefreshUserProfileOnWindowActivate()
{
    const QString token = _userSession.authToken().trimmed();
    if (token.isEmpty()) {
        return;
    }
    if (_userHydrationInFlight) {
        return;
    }
    if (_lastWindowActivateRefreshAt.isValid()
        && _lastWindowActivateRefreshAt.elapsed() < kWindowActivateRefreshThrottleMs) {
        return;
    }

    _lastWindowActivateRefreshAt.restart();
    startDirectUserHydration(token, true);
}
