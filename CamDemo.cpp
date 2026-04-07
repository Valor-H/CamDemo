#include "CamDemo.h"
#include "TitleBarUserChip.h"
#include "SARibbonBar.h"
#include "SARibbonCategory.h"
#include "SARibbonPanel.h"
#include "SARibbonSystemButtonBar.h"
#include "SARibbonQuickAccessBar.h"
#include "DesktopWeb.h"

#include <QAbstractButton>
#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QMessageBox>
#include <QSizePolicy>
#include <QTimer>
#include <QWidget>

#include <QDesktopServices>
#include <QEvent>
#include <QSettings>

CamDemo::CamDemo(QWidget* parent)
    : SARibbonMainWindow(parent)
    , _userAuth(UserModuleConfig {})
{
    ui.setupUi(this);
    setWindowTitle(tr("CamDemo"));
    setWindowIcon(QIcon(QStringLiteral(":/CamDemo/resource/logo.ico")));
    _actionNew = new QAction(QIcon(), tr("New"), this);
    _actionOpen = new QAction(QIcon(), tr("Open"), this);
    _actionSave = new QAction(QIcon(), tr("Save"), this);
    setMinimumSize(1000, 800);
    InitRibbonBar();
    InitUserChip();
    connect(_userAuth.Session(), &UserSession::AuthStateChanged, this, &CamDemo::RefreshUserChipFromSession);
    connect(_userAuth.Session(), &UserSession::UserProfileChanged, this, &CamDemo::RefreshUserChipFromSession);
    RefreshUserChipFromSession();
    _userAuth.InitFromStoredToken();
}

CamDemo::~CamDemo() = default;

bool CamDemo::event(QEvent* e)
{
    if (e && e->type() == QEvent::WindowActivate) {
        _userAuth.OnWindowActivateEvent();
    }
    return SARibbonMainWindow::event(e);
}

void CamDemo::InitRibbonBar()
{
    SARibbonBar* ribbon_bar = ribbonBar();
    if (!ribbon_bar) {
        return;
    }
    ribbon_bar->setRibbonStyle(SARibbonBar::RibbonStyleLooseThreeRow);
    SARibbonQuickAccessBar* quick_access_bar = ribbon_bar->quickAccessBar();
    quick_access_bar->addAction(_actionNew);
    quick_access_bar->addAction(_actionOpen);
    quick_access_bar->addAction(_actionSave);
    ribbon_bar->addCategoryPage(tr("File"));
    ribbon_bar->addCategoryPage(tr("Main"));
}

void CamDemo::InitUserChip()
{
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
    connect(_logoutAction, &QAction::triggered, this, &CamDemo::OnLogout);
    connect(_personalCenterAction, &QAction::triggered, this, &CamDemo::OnOpenPersonalProfile, Qt::UniqueConnection);
    connect(_teamAction, &QAction::triggered, this, &CamDemo::OnOpenTeam, Qt::UniqueConnection);

    connect(_userChip, &TitleBarUserChip::loginRequested, this, &CamDemo::OnShowAccountAuthDialog);
    connect(_userChip, &TitleBarUserChip::accountMenuRequested, this, &CamDemo::OnShowAccountMenu);
}

void CamDemo::SyncUserChipIntoTitleBar()
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

void CamDemo::RefreshUserChipFromSession()
{
    if (!_userChip) {
        return;
    }
    _userChip->SyncFromSession(_userAuth.Session());
    SyncUserChipIntoTitleBar();
    QTimer::singleShot(0, this, [this]() { SyncUserChipIntoTitleBar(); });
}

void CamDemo::OnShowAccountAuthDialog()
{
    _userAuth.ShowAccountAuthDialog(this);
}

void CamDemo::OnShowAccountMenu()
{
    if (!_userChip || !_loginMenu) {
        return;
    }
    const QPoint pos = _userChip->mapToGlobal(QPoint(0, _userChip->height()));
    _loginMenu->popup(pos);
}

void CamDemo::OnLogout()
{
    _userAuth.Logout();
}

void CamDemo::OnOpenPersonalProfile()
{
    // 直接从持久化存储读取 token，而不是从 session 读取
    // 因为 session 的 token 可能还在异步加载中
    QString tok;
    QSettings settings(_userAuth.Config().settingsOrg, _userAuth.Config().settingsApp);
    tok = settings.value(_userAuth.Config().authTokenKey).toString().trimmed();

    if (tok.isEmpty()) {
        OnShowAccountAuthDialog();
        return;
    }
    QDesktopServices::openUrl(DesktopWeb::buildPersonalProfileUrl(_userAuth.FrontendBaseUrl(), tok));
}

void CamDemo::OnOpenTeam()
{
    // 仿照“个人中心”：从本地存储读 token，避免 session 尚未加载完成
    QString tok;
    QSettings settings(_userAuth.Config().settingsOrg, _userAuth.Config().settingsApp);
    tok = settings.value(_userAuth.Config().authTokenKey).toString().trimmed();

    if (tok.isEmpty()) {
        OnShowAccountAuthDialog();
        return;
    }
    QDesktopServices::openUrl(DesktopWeb::buildTeamUrl(_userAuth.FrontendBaseUrl(), tok));
}
