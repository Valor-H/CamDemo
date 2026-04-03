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
    /** 标题栏高度与布局链同步（探测登录后 sizeHint 变化时需 invalidate，否则控件几何错误） */
    void SyncUserChipIntoTitleBar();
    void OnShowAccountAuthDialog();
    void OnShowAccountMenu();
    void OnLogout();
    void OnOpenPersonalProfile();
    void OnOpenSettingsPlaceholder();

    Ui::CamDemoClass ui;
    QAction* _actionNew;
    QAction* _actionOpen;
    QAction* _actionSave;
    UserAuthService _userAuth;
    TitleBarUserChip* _userChip { nullptr };
    QMenu* _loginMenu { nullptr };
    QAction* _personalCenterAction { nullptr };
    QAction* _settingsAction { nullptr };
    QAction* _logoutAction { nullptr };
};
