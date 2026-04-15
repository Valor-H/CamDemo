#pragma once

#include "SARibbonMainWindow.h"
#include "ui_CamDemo.h"
#include "UserAuthService.h"

class TitleBarUserChip;
class QAction;
class QMenu;

class CamDemo : public SARibbonMainWindow
{
    Q_OBJECT

public:
    CamDemo(QWidget* parent = nullptr);
    ~CamDemo() override;

protected:
    bool event(QEvent* e) override;

private slots:
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

    Ui::CamDemoClass ui;
    QAction* _actionNew;
    QAction* _actionOpen;
    QAction* _actionSave;
    qianjizn::qj_user::UserAuthService _userAuth { qianjizn::qj_user::UserModuleConfig {} };
    TitleBarUserChip* _userChip { nullptr };
    QMenu* _loginMenu { nullptr };
    QAction* _personalCenterAction { nullptr };
    QAction* _teamAction { nullptr };
    QAction* _logoutAction { nullptr };
};
