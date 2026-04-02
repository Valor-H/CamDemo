#include "CamDemo.h"
#include "TitleBarUserChip.h"
#include "SARibbonBar.h"
#include "SARibbonCategory.h"
#include "SARibbonPanel.h"
#include "SARibbonSystemButtonBar.h"
#include "SARibbonQuickAccessBar.h"
#include "LoginDialog.h"
#include "DesktopWeb.h"

#include "AuthHttpClient.h"

#include <QAbstractButton>
#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QSizePolicy>
#include <QTimer>
#include <QVariantList>
#include <QWidget>

#include <QDesktopServices>

namespace
{
const QString kSettingsOrg = QStringLiteral("QianJiZN");
const QString kSettingsApp = QStringLiteral("CamDemo");
const QString kAuthTokenKey = QStringLiteral("auth/token");
}

CamDemo::CamDemo(QWidget* parent)
    : SARibbonMainWindow(parent)
    , _userSession(this)
    , _authClient(new AuthHttpClient(QStringLiteral("http://localhost:8080"), this))
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
    connect(&_userSession, &UserSession::authStateChanged, this, &CamDemo::RefreshUserChipFromSession);
    connect(&_userSession, &UserSession::userProfileChanged, this, &CamDemo::RefreshUserChipFromSession);
    RefreshUserChipFromSession();
    InitLoginStateFromToken();
}

CamDemo::~CamDemo()
{
    if (_authClient) {
        _authClient->cancelAll();
    }
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

    _userChip = new TitleBarUserChip(bar);
    bar->addWidget(_userChip);

    QTimer::singleShot(0, this, [this]() { SyncUserChipIntoTitleBar(); });

    _loginMenu = new QMenu(this);
    _personalCenterAction = _loginMenu->addAction(tr("Personal center"));
    _settingsAction = _loginMenu->addAction(tr("Settings"));
    _logoutAction = _loginMenu->addAction(tr("Log out"));
    connect(_logoutAction, &QAction::triggered, this, &CamDemo::OnLogout);
    connect(_personalCenterAction, &QAction::triggered, this, &CamDemo::OnOpenPersonalProfile, Qt::UniqueConnection);
    connect(_settingsAction, &QAction::triggered, this, &CamDemo::OnOpenSettingsPlaceholder, Qt::UniqueConnection);

    connect(_userChip, &TitleBarUserChip::loginRequested, this, &CamDemo::OnShowLoginDialog);
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
    _userChip->relayoutInParent();
}

void CamDemo::RefreshUserChipFromSession()
{
    if (!_userChip) {
        return;
    }
    _userChip->syncFromSession(&_userSession);
    SyncUserChipIntoTitleBar();
    QTimer::singleShot(0, this, [this]() { SyncUserChipIntoTitleBar(); });
}

void CamDemo::OnShowLoginDialog()
{
    LoginDialog dlg(this);
    connect(&dlg, &LoginDialog::loginSucceeded, this, &CamDemo::OnLoginSucceeded);
    dlg.exec();
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
    _authClient->cancelAll();
    ClearAuthTokenFromSettings();
    _userSession.logout();
}

void CamDemo::OnLoginSucceeded(const QVariantMap& payload)
{
    _authClient->cancelAll();
    SaveAuthTokenToSettings(payload.value(QStringLiteral("token")).toString());
    _userSession.applyFromLoginPayload(payload);
}


void CamDemo::StartDirectUserHydration(const QString& token, bool allowRefresh)
{
    const QString trimmed = token.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }

    _authClient->cancelAll();
    FetchCurrentUserDirect(trimmed, allowRefresh);
}

void CamDemo::FetchCurrentUserDirect(const QString& token, bool allowRefresh)
{
    _authClient->post(QStringLiteral("/api/user/current"), token, 10,
        [this, token, allowRefresh](const AuthHttpClient::Response& resp) {
            if (!resp.networkOk) {
                // 网络失败时保留 token-only，会话稍后可重试。
                return;
            }

            if (resp.bizCode == 200) {
                const QVariantMap userMap = resp.data.value(QStringLiteral("user")).toMap();
                if (userMap.isEmpty()) {
                    return;
                }

                QVariantMap payload;
                payload.insert(QStringLiteral("token"), token);
                payload.insert(QStringLiteral("user"), userMap);
                _userSession.applyFromLoginPayload(payload);
                RefreshUserChipFromSession();
                return;
            }

            if (resp.bizCode == 401 && allowRefresh) {
                RefreshTokenDirectAndRetry(token);
                return;
            }

            if (resp.bizCode == 401) {
                _authClient->cancelAll();
                ClearAuthTokenFromSettings();
                _userSession.logout();
            }
        });
}

void CamDemo::RefreshTokenDirectAndRetry(const QString& token)
{
    _authClient->post(QStringLiteral("/api/auth/refresh"), token, 10,
        [this, token](const AuthHttpClient::Response& resp) {
            if (!resp.networkOk) {
                // 刷新链路网络失败，保持 token-only，允许后续重试。
                return;
            }

            if (resp.bizCode == 200) {
                FetchCurrentUserDirect(token, false);
                return;
            }

            if (resp.bizCode == 401) {
                _authClient->cancelAll();
                ClearAuthTokenFromSettings();
                _userSession.logout();
            }
        });
}

void CamDemo::SaveAuthTokenToSettings(const QString& token)
{
    QSettings settings(kSettingsOrg, kSettingsApp);
    const QString trimmed = token.trimmed();
    if (trimmed.isEmpty()) {
        settings.remove(kAuthTokenKey);
    } else {
        settings.setValue(kAuthTokenKey, trimmed);
    }
    settings.sync();
}

void CamDemo::InitLoginStateFromToken()
{
    const QString token = LoadAuthTokenFromSettings();
    if (token.isEmpty()) {
        return;
    }

    QVariantMap data;
    data.insert(QStringLiteral("token"), token);
    data.insert(QStringLiteral("loggedIn"), true);
    _userSession.applyFromProbe(data);
    RefreshUserChipFromSession();
    StartDirectUserHydration(token, true);
}

QString CamDemo::LoadAuthTokenFromSettings() const
{
    QSettings settings(kSettingsOrg, kSettingsApp);
    return settings.value(kAuthTokenKey).toString().trimmed();
}

void CamDemo::ClearAuthTokenFromSettings()
{
    QSettings settings(kSettingsOrg, kSettingsApp);
    settings.remove(kAuthTokenKey);
    settings.sync();
}

void CamDemo::OnOpenPersonalProfile()
{
    const QString tok = _userSession.authToken().trimmed();
    if (tok.isEmpty()) {
        OnShowLoginDialog();
        return;
    }
    QDesktopServices::openUrl(DesktopWeb::BuildPersonalProfileUrl(tok));
}

void CamDemo::OnOpenSettingsPlaceholder()
{
    QMessageBox::information(this, tr("Settings"), tr("Settings page is not available yet."));
}
