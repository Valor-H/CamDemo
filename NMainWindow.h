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

    bool OpenFile(const QString& file_name, const QString& backup_file = "", bool silent = false);
    qianjizn::user::UserAuthService& UserAuth() { return _userAuth; }
    const qianjizn::user::UserAuthService& UserAuth() const { return _userAuth; }

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
    void OnOpenFileManager();
    void OnOpenTeam();

    QAction* _actionNew { nullptr };
    QAction* _actionOpen { nullptr };
    QAction* _actionSave { nullptr };
    qianjizn::user::UserAuthService _userAuth { qianjizn::user::UserModuleConfig {} };
    TitleBarUserChip* _userChip { nullptr };
    QMenu* _loginMenu { nullptr };
    QAction* _personalCenterAction { nullptr };
    QAction* _fileManagerAction { nullptr };
    QAction* _teamAction { nullptr };
    QAction* _logoutAction { nullptr };
    int _cefAuthRetryCount { 0 };
};