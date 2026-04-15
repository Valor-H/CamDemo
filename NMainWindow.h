#pragma once

#include "SARibbonMainWindow.h"
#include "UserAuthService.h"

class TitleBarUserChip;
class QAction;
class QMenu;
class QEvent;

class NMainWindow : public SARibbonMainWindow
{
    Q_OBJECT

public:
    explicit NMainWindow(QWidget* parent = nullptr);
    ~NMainWindow() override;

protected:
    bool event(QEvent* e) override;
    void InitializeMainWindowShell();

protected slots:
    void RefreshUserChipFromSession();

private:
    void InitRibbonBar();
    void InitUserChip();
    void SyncUserChipIntoTitleBar();
    void OnShowAccountAuthDialog();
    void OnShowAccountMenu();
    void OnLogout();
    void OnOpenPersonalProfile();
    void OnOpenTeam();

    QAction* _actionNew { nullptr };
    QAction* _actionOpen { nullptr };
    QAction* _actionSave { nullptr };
    qianjizn::qj_user::UserAuthService _userAuth { qianjizn::qj_user::UserModuleConfig {} };
    TitleBarUserChip* _userChip { nullptr };
    QMenu* _loginMenu { nullptr };
    QAction* _personalCenterAction { nullptr };
    QAction* _teamAction { nullptr };
    QAction* _logoutAction { nullptr };
    int _cefAuthRetryCount { 0 };
};