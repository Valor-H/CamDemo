#include "CamDemo.h"
#include "TitleBarUserChip.h"
#include "SARibbonBar.h"
#include "SARibbonCategory.h"
#include "SARibbonPanel.h"
#include "SARibbonSystemButtonBar.h"
#include "SARibbonQuickAccessBar.h"
#include "LoginDialog.h"
#include "DesktopWeb.h"

#include <QAbstractButton>
#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QMessageBox>
#include <QSizePolicy>
#include <QTimer>
#include <QVariantList>
#include <QWidget>

#include <QCefSetting.h>
#include <QCefView.h>
#include <QDesktopServices>

CamDemo::CamDemo(QWidget* parent)
    : SARibbonMainWindow(parent)
    , _userSession(this)
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
    _tokenClearPending = false;
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
    _userSession.logout();
    ClearWebAuthToken();
}

void CamDemo::OnLoginSucceeded(const QVariantMap& payload)
{
    _userSession.applyFromLoginPayload(payload);
}

void CamDemo::ClearWebAuthToken()
{
    DisposeTokenProbeView();
    _tokenProbePending = false;
    _tokenClearPending = true;

    QCefSetting setting;
    const QString loginUrl = DesktopWeb::BuildDesktopLoginUrl();
    _tokenProbeView = new QCefView(loginUrl, &setting, this);
    _tokenProbeView->hide();

    connect(_tokenProbeView,
            &QCefView::loadEnd,
            this,
            [this, loginUrl](const QCefBrowserId&, const QCefFrameId&, bool isMainFrame, int) {
                if (!isMainFrame || !_tokenProbeView) {
                    return;
                }
                static const QString kScript = QStringLiteral(R"JS(
                    (() => {
                      let ok = false;
                      let error = '';
                      try {
                        if (window.localStorage) {
                          window.localStorage.removeItem('auth_token');
                          ok = true;
                        }
                      } catch (e) {
                        error = e && e.message ? String(e.message) : String(e);
                      }
                      try {
                        if (window.CallBridge && typeof window.CallBridge.invoke === 'function') {
                          window.CallBridge.invoke('Desktop.TokenCleared', { ok, error });
                        }
                      } catch (e) {
                      }
                    })();
                )JS");
                _tokenProbeView->executeJavascript(QCefView::MainFrameID, kScript, loginUrl);
                QTimer::singleShot(1500, this, [this]() {
                    if (_tokenClearPending) {
                        _tokenClearPending = false;
                        DisposeTokenProbeView();
                    }
                });
            });

    connect(_tokenProbeView,
            &QCefView::invokeMethod,
            this,
            [this](const QCefBrowserId&, const QCefFrameId&, const QString& method, const QVariantList& arguments) {
                if (method != QStringLiteral("Desktop.TokenCleared") || !_tokenClearPending) {
                    return;
                }
                QVariantMap data;
                if (!arguments.isEmpty()) {
                    data = arguments.front().toMap();
                }
                _tokenClearPending = false;
                OnTokenCleared(data);
                DisposeTokenProbeView();
            });
}

void CamDemo::InitLoginStateFromToken()
{
    if (_tokenProbePending) {
        return;
    }
    _tokenProbePending = true;
    DisposeTokenProbeView();
    _tokenClearPending = false;

    QCefSetting setting;
    const QString loginUrl = DesktopWeb::BuildDesktopLoginUrl();
    _tokenProbeView = new QCefView(loginUrl, &setting, this);
    _tokenProbeView->hide();

    connect(_tokenProbeView,
            &QCefView::loadEnd,
            this,
            [this, loginUrl](const QCefBrowserId&, const QCefFrameId&, bool isMainFrame, int) {
                if (!isMainFrame || !_tokenProbeView) {
                    return;
                }
                static const QString kScript = QStringLiteral(R"JS(
                    (() => {
                      let storage = null;
                      try {
                        storage = window.localStorage;
                      } catch (e) {
                        storage = null;
                      }
                      const token = storage ? (storage.getItem('auth_token') || '') : '';

                      const payload = {
                        token: token,
                        loggedIn: !!token,
                        user: {}
                      };

                      try {
                        const raw = storage ? (storage.getItem('current_user_cache') || '') : '';
                        if (raw) {
                          const parsed = JSON.parse(raw);
                          if (parsed && typeof parsed === 'object' && !Array.isArray(parsed)) {
                            payload.user = parsed;
                          }
                        }
                      } catch (e) {
                      }

                      try {
                        if (window.CallBridge && typeof window.CallBridge.invoke === 'function') {
                          window.CallBridge.invoke('Desktop.InitUserProbe', payload);
                        }
                      } catch (e) {
                      }
                    })();
                )JS");
                _tokenProbeView->executeJavascript(QCefView::MainFrameID, kScript, loginUrl);
            });

    connect(_tokenProbeView,
            &QCefView::invokeMethod,
            this,
            [this](const QCefBrowserId&, const QCefFrameId&, const QString& method, const QVariantList& arguments) {
                if (method != QStringLiteral("Desktop.InitUserProbe") || !_tokenProbePending) {
                    return;
                }
                QVariantMap data;
                if (!arguments.isEmpty()) {
                    data = arguments.front().toMap();
                }
                _tokenProbePending = false;
                ApplyUserInfoFromMap(data);
                DisposeTokenProbeView();
            });
}

void CamDemo::DisposeTokenProbeView()
{
    if (!_tokenProbeView) {
        return;
    }
    _tokenProbeView->deleteLater();
    _tokenProbeView = nullptr;
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

void CamDemo::ApplyUserInfoFromMap(const QVariantMap& data)
{
    _userSession.applyFromProbe(data);
}

void CamDemo::OnTokenCleared(const QVariantMap& data)
{
    Q_UNUSED(data);
}
