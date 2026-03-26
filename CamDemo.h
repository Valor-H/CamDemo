#pragma once

#include "SARibbonMainWindow.h"
#include "ui_CamDemo.h"
#include <QToolButton>
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
    Ui::CamDemoClass ui;
    QAction* _actionNew;
    QAction* _actionOpen;
    QAction* _actionSave;
    QToolButton* _loginButton;
    QMenu* _loginMenu;
    QAction* _personalAccountAction;
    QAction* _feedbackAction;
    QAction* _exitAction;
    bool _isLoggedIn;
};

