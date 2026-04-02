#pragma once

#include "SARibbonMainWindow.h"
#include "ui_CamDemo.h"
#include "UserSession.h"
#include <QVariantMap>

class TitleBarUserChip;
class QAction;
class QMenu;

class CamDemo : public SARibbonMainWindow
{
    Q_OBJECT

public:
    CamDemo(QWidget* parent = nullptr);
    ~CamDemo();

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
    void ClearWebAuthToken();
    void DisposeTokenProbeView();
    void ApplyUserInfoFromMap(const QVariantMap& data);
    void OnTokenCleared(const QVariantMap& data);
    void OnOpenPersonalProfile();
    void OnOpenSettingsPlaceholder();

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
    class QCefView* _tokenProbeView { nullptr };
    bool _tokenProbePending { false };
    bool _tokenClearPending { false };
};
