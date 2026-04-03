#pragma once

#include "UserModuleConfig.h"
#include "UserSession.h"

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QUrl>

class AuthHttpClient;

/**
 * 登录注册相关门面：持久化 token、拉取当前用户、刷新 token、窗口激活节流补水。
 * 展示登录对话框前宿主须已完成 QCef 全局初始化。
 */
class UserAuthService final : public QObject
{
    Q_OBJECT

public:
    explicit UserAuthService(const UserModuleConfig& cfg, QObject* parent = nullptr);
    ~UserAuthService() override;

    UserSession* session() { return &_userSession; }
    const UserSession* session() const { return &_userSession; }

    const UserModuleConfig& config() const { return _cfg; }
    QUrl apiBaseUrl() const { return _cfg.apiBaseUrl; }
    QUrl frontendBaseUrl() const { return _cfg.frontendBaseUrl; }

    void cancelAllPendingRequests();

    void showLoginDialog(QWidget* parent);
    void logout();
    void initFromStoredToken();
    void onWindowActivateEvent();

private slots:
    void onLoginSucceeded(const QVariantMap& payload);
    void tryRefreshUserProfileOnWindowActivate();

private:
    void scheduleWindowActivateRefresh();
    void saveAuthTokenToSettings(const QString& token);
    QString loadAuthTokenFromSettings() const;
    void clearAuthTokenFromSettings();

    void startDirectUserHydration(const QString& token, bool allowRefresh);
    void fetchCurrentUserDirect(const QString& token, bool allowRefresh);
    void refreshTokenDirectAndRetry(const QString& token);

    UserModuleConfig _cfg;
    UserSession _userSession;
    AuthHttpClient* _authClient { nullptr };
    QTimer* _windowActivateRefreshDebounceTimer { nullptr };
    QElapsedTimer _lastWindowActivateRefreshAt;
    bool _userHydrationInFlight { false };
};
