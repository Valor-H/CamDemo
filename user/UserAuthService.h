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
 * 展示账户认证对话框前宿主须已完成 QCef 全局初始化。
 */
class UserAuthService final : public QObject
{
    Q_OBJECT

public:
    explicit UserAuthService(const UserModuleConfig& cfg, QObject* parent = nullptr);
    ~UserAuthService() override;

    UserSession* Session() { return &_userSession; }
    const UserSession* Session() const { return &_userSession; }

    const UserModuleConfig& Config() const { return _cfg; }
    QUrl ApiBaseUrl() const { return _cfg.apiBaseUrl; }
    QUrl FrontendBaseUrl() const { return _cfg.frontendBaseUrl; }

    void CancelAllPendingRequests();

    void ShowAccountAuthDialog(QWidget* parent);
    void Logout();
    void InitFromStoredToken();
    void OnWindowActivateEvent();

private slots:
    void OnLoginSucceeded(const QVariantMap& payload);
    void TryRefreshUserProfileOnWindowActivate();

private:
    void ScheduleWindowActivateRefresh();
    void SaveAuthTokenToSettings(const QString& token);
    QString LoadAuthTokenFromSettings() const;
    void ClearAuthTokenFromSettings();

    void StartDirectUserHydration(const QString& token, bool allowRefresh);
    void FetchCurrentUserDirect(const QString& token, bool allowRefresh);
    void RefreshTokenDirectAndRetry(const QString& token);

    UserModuleConfig _cfg;
    UserSession _userSession;
    AuthHttpClient* _authClient { nullptr };
    QTimer* _windowActivateRefreshDebounceTimer { nullptr };
    QElapsedTimer _lastWindowActivateRefreshAt;
    bool _userHydrationInFlight { false };
};
