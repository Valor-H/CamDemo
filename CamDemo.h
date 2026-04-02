#pragma once

#include "SARibbonMainWindow.h"
#include "ui_CamDemo.h"
#include "UserSession.h"
#include <QElapsedTimer>
#include <QVariantMap>

class AuthHttpClient;

class TitleBarUserChip;
class QAction;
class QMenu;

class CamDemo : public SARibbonMainWindow
{
    Q_OBJECT

public:
    CamDemo(QWidget* parent = nullptr);
    ~CamDemo();

protected:
    bool event(QEvent* e) override;

private slots:
    void RefreshUserChipFromSession();

private:
    void InitRibbonBar();
    void InitUserChip();
    /** 标题栏高度与布局链同步（探测登录后 sizeHint 变化时需 invalidate，否则控件几何错误） */
    void SyncUserChipIntoTitleBar();
    void OnShowLoginDialog();
    void OnShowAccountMenu();
    void OnLogout();
    void OnLoginSucceeded(const QVariantMap& payload);
    void InitLoginStateFromToken();
    void StartDirectUserHydration(const QString& token, bool allowRefresh);
    void FetchCurrentUserDirect(const QString& token, bool allowRefresh);
    void RefreshTokenDirectAndRetry(const QString& token);
    void SaveAuthTokenToSettings(const QString& token);
    QString LoadAuthTokenFromSettings() const;
    void ClearAuthTokenFromSettings();
    void OnOpenPersonalProfile();
    void OnOpenSettingsPlaceholder();
    void ScheduleWindowActivateRefresh();
    void TryRefreshUserProfileOnWindowActivate();

    Ui::CamDemoClass ui;
    QAction* _actionNew;
    QAction* _actionOpen;
    QAction* _actionSave;
    UserSession _userSession;
    TitleBarUserChip* _userChip { nullptr };
    QMenu* _loginMenu { nullptr };
    QAction* _personalCenterAction { nullptr };
    QAction* _settingsAction { nullptr };
    QAction* _logoutAction { nullptr };
    AuthHttpClient* _authClient { nullptr };
    QTimer* _windowActivateRefreshDebounceTimer { nullptr };
    QElapsedTimer _lastWindowActivateRefreshAt;
    bool _userHydrationInFlight { false };
};
