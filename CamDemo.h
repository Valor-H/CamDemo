#pragma once

#include "SARibbonMainWindow.h"
#include "ui_CamDemo.h"
#include <QString>
#include <QToolButton>
#include <QVariantMap>
class CamDemo : public SARibbonMainWindow
{
    Q_OBJECT

public:
    CamDemo(QWidget *parent = nullptr);
    ~CamDemo();

private:
    void InitRibbonBar();
    void InitWindowLoginButton();
    void UpdateLoginButtonState();
    void OnShowLoginDialog();
    void OnShowMenu();
    void OnLogout();
    void OnLoginSucceeded(const QVariantMap& payload);
    void InitLoginStateFromToken();
    void ClearWebAuthToken();
    void DisposeTokenProbeView();
    void ApplyUserInfoFromMap(const QVariantMap& data);
    void OnTokenCleared(const QVariantMap& data);
    Ui::CamDemoClass ui;
    QAction* _actionNew;
    QAction* _actionOpen;
    QAction* _actionSave;
    QToolButton* _loginButton;
    QMenu* _loginMenu;
    QAction* _personalAccountAction;
    QAction* _feedbackAction;
    QAction* _exitAction;
    QString _displayName;
    bool _isLoggedIn;
    // 当前用户完整信息（字段名与后端 Java 实体保持一致：uuid/userName/nickName/email/phone/sex/avatar/role）
    QVariantMap _currentUser;
    class QCefView* _tokenProbeView;
    bool _tokenProbePending;
    bool _tokenClearPending;
};

