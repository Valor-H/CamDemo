#include "NMainWindow.h"

#include "DesktopWeb.h"
#include "SARibbonBar.h"
#include "SARibbonQuickAccessBar.h"
#include "SARibbonSystemButtonBar.h"
#include "TitleBarUserChip.h"

#include <QAbstractButton>
#include <QAction>
#include <QDesktopServices>
#include <QEvent>
#include <QIcon>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QSizePolicy>
#include <QTimer>
#include <QWidget>

#include <QCefContext.h>

using qianjizn::qj_user::UserSession;

NMainWindow::NMainWindow(QWidget* parent)
    : SARibbonMainWindow(parent)
{
    _actionNew = new QAction(QIcon(), tr("New"), this);
    _actionOpen = new QAction(QIcon(), tr("Open"), this);
    _actionSave = new QAction(QIcon(), tr("Save"), this);

    connect(_userAuth.Session(), &UserSession::AuthStateChanged, this, &NMainWindow::RefreshUserChipFromSession);
    connect(_userAuth.Session(), &UserSession::UserProfileChanged, this, &NMainWindow::RefreshUserChipFromSession);
}

NMainWindow::~NMainWindow() = default;

bool NMainWindow::event(QEvent* e)
{
    if (e && e->type() == QEvent::WindowActivate) {
        _userAuth.OnWindowActivateEvent();
    }
    return SARibbonMainWindow::event(e);
}

void NMainWindow::InitializeMainWindowShell()
{
    InitRibbonBar();
    InitUserChip();
    RefreshUserChipFromSession();
    _userAuth.InitFromStoredToken();
}

void NMainWindow::RefreshUserChipFromSession()
{
    if (!_userChip) {
        return;
    }
    _userChip->SyncFromSession(_userAuth.Session());
    SyncUserChipIntoTitleBar();
    QTimer::singleShot(0, this, [this]() { SyncUserChipIntoTitleBar(); });
}

void NMainWindow::InitRibbonBar()
{
    SARibbonBar* ribbonBarWidget = ribbonBar();
    if (!ribbonBarWidget) {
        return;
    }
    ribbonBarWidget->setRibbonStyle(SARibbonBar::RibbonStyleLooseThreeRow);
    SARibbonQuickAccessBar* quickAccessBar = ribbonBarWidget->quickAccessBar();
    quickAccessBar->addAction(_actionNew);
    quickAccessBar->addAction(_actionOpen);
    quickAccessBar->addAction(_actionSave);
    ribbonBarWidget->addCategoryPage(tr("File"));
    ribbonBarWidget->addCategoryPage(tr("Main"));
}

void NMainWindow::InitUserChip()
{
    if (_userChip) {
        return;
    }

    SARibbonSystemButtonBar* bar = windowButtonBar();
    if (!bar) {
        return;
    }
    QWidget* spacer = new QWidget(bar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    bar->addWidget(spacer);

    _userChip = new TitleBarUserChip(bar, _userAuth.ApiBaseUrl());
    bar->addWidget(_userChip);

    QTimer::singleShot(0, this, [this]() { SyncUserChipIntoTitleBar(); });

    _loginMenu = new QMenu(this);
    _personalCenterAction = _loginMenu->addAction(tr("Personal center"));
    _teamAction = _loginMenu->addAction(tr("管理团队"));
    _logoutAction = _loginMenu->addAction(tr("Log out"));
    connect(_logoutAction, &QAction::triggered, this, &NMainWindow::OnLogout);
    connect(_personalCenterAction, &QAction::triggered, this, &NMainWindow::OnOpenPersonalProfile, Qt::UniqueConnection);
    connect(_teamAction, &QAction::triggered, this, &NMainWindow::OnOpenTeam, Qt::UniqueConnection);

    connect(_userChip, &TitleBarUserChip::loginRequested, this, &NMainWindow::OnShowAccountAuthDialog);
    connect(_userChip, &TitleBarUserChip::accountMenuRequested, this, &NMainWindow::OnShowAccountMenu);
}

void NMainWindow::SyncUserChipIntoTitleBar()
{
    if (!_userChip) {
        return;
    }
    SARibbonSystemButtonBar* bar = windowButtonBar();
    if (!bar) {
        return;
    }
    int rowH = bar->windowTitleHeight();
    if (QAbstractButton* minBtn = bar->minimizeButton()) {
        const int mh = minBtn->height();
        if (mh > 0) {
            rowH = mh;
        }
    }
    const int minH = TitleBarUserChip::kAvatarSide;
    const int h = rowH > 0 ? qMax(rowH, minH) : minH;
    _userChip->setFixedHeight(h);
    _userChip->RelayoutInParent();
}

void NMainWindow::OnShowAccountAuthDialog()
{
    constexpr int kMaxRetries = 40;
    constexpr int kRetryMs = 50;

    if (!QCefContext::instance()) {
        if (_cefAuthRetryCount >= kMaxRetries) {
            _cefAuthRetryCount = 0;
            QMessageBox::warning(this, tr("提示"), tr("浏览器内核未就绪，请稍后重试。"));
            return;
        }
        ++_cefAuthRetryCount;
        QTimer::singleShot(kRetryMs, this, &NMainWindow::OnShowAccountAuthDialog);
        return;
    }
    _cefAuthRetryCount = 0;
    _userAuth.ShowAccountAuthDialog(this);
}

void NMainWindow::OnShowAccountMenu()
{
    if (!_userChip || !_loginMenu) {
        return;
    }
    const QPoint pos = _userChip->mapToGlobal(QPoint(0, _userChip->height()));
    _loginMenu->popup(pos);
}

void NMainWindow::OnLogout()
{
    _userAuth.Logout();
}

void NMainWindow::OnOpenPersonalProfile()
{
    QString token;
    QSettings settings(_userAuth.Config().settingsOrg, _userAuth.Config().settingsApp);
    token = settings.value(_userAuth.Config().authTokenKey).toString().trimmed();

    if (token.isEmpty()) {
        OnShowAccountAuthDialog();
        return;
    }
    QDesktopServices::openUrl(qianjizn::qj_user::buildPersonalProfileUrl(_userAuth.FrontendBaseUrl(), token));
}

void NMainWindow::OnOpenTeam()
{
    QString token;
    QSettings settings(_userAuth.Config().settingsOrg, _userAuth.Config().settingsApp);
    token = settings.value(_userAuth.Config().authTokenKey).toString().trimmed();

    if (token.isEmpty()) {
        OnShowAccountAuthDialog();
        return;
    }
    QDesktopServices::openUrl(qianjizn::qj_user::buildTeamUrl(_userAuth.FrontendBaseUrl(), token));
}