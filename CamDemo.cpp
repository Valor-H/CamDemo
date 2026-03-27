#include "CamDemo.h"
#include "SARibbonBar.h"
#include "SARibbonCategory.h"
#include "SARibbonPanel.h"
#include "SARibbonSystemButtonBar.h"
#include "SARibbonQuickAccessBar.h"
#include "LoginDialog.h"

#include <QAbstractButton>
#include <QIcon>
#include <QSizePolicy>
#include <QTimer>
#include <QToolButton>
#include <QWidget>

CamDemo::CamDemo(QWidget *parent)
    : SARibbonMainWindow(parent)
{
    ui.setupUi(this);
    setWindowTitle(tr("CamDemo"));
    setWindowIcon(QIcon(QStringLiteral(":/CamDemo/resource/logo.ico")));
    _actionNew = new QAction(QIcon(), tr("New"), this);
    _actionOpen = new QAction(QIcon(), tr("Open"), this);
    _actionSave = new QAction(QIcon(), tr("Save"), this);
    setMinimumSize(1000, 800);
    InitRibbonBar();
    InitWindowLoginButton();
}

CamDemo::~CamDemo()
{}

void CamDemo::InitRibbonBar()
{
    SARibbonBar* ribbon_bar = ribbonBar();
    if (!ribbon_bar) {
        return;
    }
    ribbon_bar->setRibbonStyle(SARibbonBar::RibbonStyleLooseThreeRow);
    // Add actions to QuickAccessBar (right side of logo)
    SARibbonQuickAccessBar* quick_access_bar = ribbon_bar->quickAccessBar();
    quick_access_bar->addAction(_actionNew);
    quick_access_bar->addAction(_actionOpen);
    quick_access_bar->addAction(_actionSave);
    SARibbonCategory* category_file = ribbon_bar->addCategoryPage(tr("File"));
    SARibbonCategory* category_main = ribbon_bar->addCategoryPage(tr("Main"));
}

void CamDemo::InitWindowLoginButton()
{
    SARibbonSystemButtonBar* bar = windowButtonBar();
    if (!bar) {
        return;
    }
    QWidget* spacer = new QWidget(bar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    bar->addWidget(spacer);

    QToolButton* login_button = new QToolButton(bar);
    login_button->setObjectName(QStringLiteral("SAUserAvatarButton"));
    login_button->setAutoRaise(true);
    login_button->setIcon(QIcon(QStringLiteral(":/CamDemo/resource/avatar.png")));
    login_button->setText(tr("Not logged in"));
    login_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    login_button->setFocusPolicy(Qt::NoFocus);
    bar->addWidget(login_button);

    QTimer::singleShot(0, this, [bar, login_button]() {
        int rowH = bar->windowTitleHeight();
        if (QAbstractButton* minBtn = bar->minimizeButton()) {
            const int mh = minBtn->height();
            if (mh > 0) {
                rowH = mh;
            }
        }
        if (rowH > 0) {
            login_button->setFixedHeight(rowH);
        }
        const int iconDim = qMax(16, rowH - 8);
        login_button->setIconSize(QSize(iconDim, iconDim));
        login_button->setMinimumWidth(login_button->sizeHint().width());
    });

    // 初始化登录菜单和动作
    _loginButton = login_button;
    _loginMenu = new QMenu(this);
    _personalAccountAction = _loginMenu->addAction(tr("Personal Account"));
    _feedbackAction = _loginMenu->addAction(tr("Feedback"));
    _exitAction = _loginMenu->addAction(tr("Exit"));
    _loginButton->setMenu(_loginMenu);

    // 设置初始登录状态为 false
    _isLoggedIn = false;
    UpdateLoginButtonState();
}

void CamDemo::UpdateLoginButtonState()
{
    if (_isLoggedIn)
    {
        _personalAccountAction->setVisible(true);
        _feedbackAction->setVisible(true);
        _exitAction->setVisible(true);       
        disconnect(_loginButton, &QToolButton::clicked, this, &CamDemo::OnShowLoginDialog);
        connect(_loginButton, &QToolButton::clicked, this, &CamDemo::OnShowMenu);
    }
    else
    {
        _personalAccountAction->setVisible(false);
        _feedbackAction->setVisible(false);
        _exitAction->setVisible(false);
        disconnect(_loginButton, &QToolButton::clicked, this, &CamDemo::OnShowMenu);
        connect(_loginButton, &QToolButton::clicked, this, &CamDemo::OnShowLoginDialog);
    }
}

void CamDemo::OnShowLoginDialog()
{
	LoginDialog dlg(this);
	dlg.exec();
}

void CamDemo::OnShowMenu()
{
    QPoint pos = _loginButton->mapToGlobal(QPoint(0, _loginButton->height()));
    _loginMenu->popup(pos);
}