#include "CamDemo.h"
#include "SARibbonBar.h"
#include "SARibbonCategory.h"
#include "SARibbonPanel.h"
#include "SARibbonSystemButtonBar.h"
#include "SARibbonQuickAccessBar.h"
#include "LoginDialog.h"
#include "DesktopWeb.h"

#include <QAbstractButton>
#include <QIcon>
#include <QSizePolicy>
#include <QTimer>
#include <QToolButton>
#include <QVariantList>
#include <QWidget>

#include <QCefSetting.h>
#include <QCefView.h>

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
    // 为菜单指示三角预留固定区域，避免在窄宽度下被文本挤压。
    login_button->setStyleSheet(QStringLiteral(
        "QToolButton#SAUserAvatarButton {"
        "  padding-right: 18px;"
        "}"
        "QToolButton#SAUserAvatarButton::menu-indicator {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: right center;"
        "  right: 6px;"
        "  width: 10px;"
        "  height: 10px;"
        "}"
    ));
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
        // 在 sizeHint 基础上增加少量冗余，确保右侧三角始终完整显示。
        login_button->setMinimumWidth(login_button->sizeHint().width() + 12);
    });

    // UI labels are translated via ts/qm resources.
    _loginButton = login_button;
    _loginMenu = new QMenu(this);
    _personalAccountAction = _loginMenu->addAction(tr("Personal Account"));
    _feedbackAction = _loginMenu->addAction(tr("Feedback"));
    _exitAction = _loginMenu->addAction(tr("Exit"));
    connect(_exitAction, &QAction::triggered, this, &CamDemo::OnLogout);
    _loginButton->setMenu(_loginMenu);

    // Initial login state is false.
    _isLoggedIn = false;
    _displayName.clear();
    _tokenProbeView = nullptr;
    _tokenProbePending = false;
    UpdateLoginButtonState();
}

void CamDemo::UpdateLoginButtonState()
{
    // 关键修复：每次刷新状态都先断开两个点击槽，避免重复 connect 导致一次点击触发多次弹窗。
    disconnect(_loginButton, &QToolButton::clicked, this, &CamDemo::OnShowLoginDialog);
    disconnect(_loginButton, &QToolButton::clicked, this, &CamDemo::OnShowMenu);

    if (_isLoggedIn)
    {
        // 已登录时优先显示昵称，昵称缺失时回退到固定文案。
        _loginButton->setText(_displayName.isEmpty() ? tr("Logged in") : _displayName);
        _personalAccountAction->setVisible(true);
        _feedbackAction->setVisible(true);
        _exitAction->setVisible(true);
        connect(_loginButton, &QToolButton::clicked, this, &CamDemo::OnShowMenu, Qt::UniqueConnection);
    }
    else
    {
        _loginButton->setText(tr("Not logged in"));
        _personalAccountAction->setVisible(false);
        _feedbackAction->setVisible(false);
        _exitAction->setVisible(false);
        connect(_loginButton, &QToolButton::clicked, this, &CamDemo::OnShowLoginDialog, Qt::UniqueConnection);
    }
}

void CamDemo::OnShowLoginDialog()
{
    LoginDialog dlg(this);
    connect(&dlg, &LoginDialog::loginSucceeded, this, &CamDemo::OnLoginSucceeded);
    dlg.exec();
}

void CamDemo::OnShowMenu()
{
    QPoint pos = _loginButton->mapToGlobal(QPoint(0, _loginButton->height()));
    _loginMenu->popup(pos);
}

void CamDemo::OnLogout()
{
    // 先更新本地 UI 状态，确保用户点击退出后立即回到未登录态。
    _isLoggedIn = false;
    _displayName.clear();
    UpdateLoginButtonState();
    // 同步清理 CEF 存储中的 token，避免重启后被探测为已登录。
    ClearWebAuthToken();
}

void CamDemo::OnLoginSucceeded(const QVariantMap& payload)
{
    const QString token = payload.value(QStringLiteral("token")).toString().trimmed();
    if (token.isEmpty()) {
        // 兼容兜底：若异常情况下未收到 token，退回启动探测逻辑，避免 UI 假登录。
        _isLoggedIn = false;
        _displayName.clear();
        _currentUser.clear();
        UpdateLoginButtonState();
        InitLoginStateFromToken();
        return;
    }

    const QVariantMap user = payload.value(QStringLiteral("user")).toMap();
    // 登录成功后保存完整用户对象，便于后续模块复用。
    if (!user.isEmpty()) {
        _currentUser = user;
    }
    // 登录成功后优先使用昵称，无昵称时回退用户名，再回退固定文案。
    const QString nickName = user.value(QStringLiteral("nickName")).toString().trimmed();
    const QString userName = user.value(QStringLiteral("userName")).toString().trimmed();
    _displayName = !nickName.isEmpty() ? nickName : userName;
    if (_displayName.isEmpty()) {
        _displayName = tr("Logged in");
    }
    _isLoggedIn = true;
    UpdateLoginButtonState();
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
                // 清理 token 后通过 CallBridge 回传，避免固定延时释放造成“偶发未清理”。
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
                        // ignore
                      }
                    })();
                )JS");
                _tokenProbeView->executeJavascript(QCefView::MainFrameID, kScript, loginUrl);
                // 兜底超时：避免桥接回调异常导致视图悬挂。
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

    // 启动时读取 CEF 内 localStorage 的 auth_token；executeJavascriptWithResult 在本环境不可靠，
    // 故由页面脚本通过 CallBridge.invoke('Desktop.InitUserProbe', payload) 主动回传。
    connect(_tokenProbeView,
            &QCefView::loadEnd,
            this,
            [this, loginUrl](const QCefBrowserId&, const QCefFrameId&, bool isMainFrame, int) {
                if (!isMainFrame || !_tokenProbeView) {
                    return;
                }
                static const QString kScript = QStringLiteral(R"JS(
                    (async () => {
                      // 启动探测：尽量少依赖网络，优先读 localStorage 缓存字段。
                      let storage = null;
                      try {
                        storage = window.localStorage;
                      } catch (e) {
                        storage = null;
                      }
                      const token = storage ? (storage.getItem('auth_token') || '') : '';

                      const payload = {
                        loggedIn: !!token,
                        user: {
                          uuid: '',
                          userName: '',
                          nickName: '',
                          email: '',
                          phone: '',
                          sex: 0,
                          avatar: '',
                          role: 0
                        }
                      };

                      // 优先从 localStorage 读取前端缓存的用户信息，避免 CEF 环境下网络请求偶发不稳定。
                      // 该缓存由前端在 fetchCurrentUser() 成功后写入（key: current_user_cache）。
                      try {
                        const raw = storage ? (storage.getItem('current_user_cache') || '') : '';
                        if (raw) {
                          const parsed = JSON.parse(raw);
                          payload.user.uuid = (parsed && parsed.uuid) ? String(parsed.uuid) : '';
                          payload.user.userName = (parsed && parsed.userName) ? String(parsed.userName) : '';
                          payload.user.nickName = (parsed && parsed.nickName) ? String(parsed.nickName) : '';
                          payload.user.email = (parsed && parsed.email) ? String(parsed.email) : '';
                          payload.user.phone = (parsed && parsed.phone) ? String(parsed.phone) : '';
                          payload.user.sex = (parsed && typeof parsed.sex === 'number') ? parsed.sex : 0;
                          payload.user.avatar = (parsed && parsed.avatar) ? String(parsed.avatar) : '';
                          payload.user.role = (parsed && typeof parsed.role === 'number') ? parsed.role : 0;
                        }
                      } catch (e) {
                        // ignore
                      }

                      // 兜底：有 token 且缓存未命中时，尝试拉取当前用户（可能受代理/同源影响，失败不影响启动）。
                      if (token && !payload.user.uuid && !payload.user.nickName && !payload.user.userName) {
                        try {
                          const resp = await fetch('/api/user/current', {
                            method: 'POST',
                            headers: {
                              'Content-Type': 'application/json',
                              'Authorization': 'Bearer ' + token
                            }
                          });
                          const json = await resp.json().catch(() => null);
                          const user = json && json.data && json.data.user ? json.data.user : null;
                          if (user) {
                            payload.user.uuid = user.uuid || '';
                            payload.user.userName = user.userName || '';
                            payload.user.nickName = user.nickName || '';
                            payload.user.email = user.email || '';
                            payload.user.phone = user.phone || '';
                            payload.user.sex = typeof user.sex === 'number' ? user.sex : 0;
                            payload.user.avatar = user.avatar || '';
                            payload.user.role = typeof user.role === 'number' ? user.role : 0;
                          }
                        } catch (e) {
                          // ignore
                        }
                      }

                      try {
                        if (window.CallBridge && typeof window.CallBridge.invoke === 'function') {
                          window.CallBridge.invoke('Desktop.InitUserProbe', payload);
                        }
                      } catch (e) {
                        /* 桥接失败时无 Qt 回调，保持静默；界面保持未登录 */
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

void CamDemo::ApplyUserInfoFromMap(const QVariantMap& data)
{
    const bool loggedIn = data.value(QStringLiteral("loggedIn")).toBool();
    if (!loggedIn) {
        _isLoggedIn = false;
        _displayName.clear();
        _currentUser.clear();
        UpdateLoginButtonState();
        return;
    }

    _isLoggedIn = true;
    const QVariantMap user = data.value(QStringLiteral("user")).toMap();
    const QString uuid = user.value(QStringLiteral("uuid")).toString();
    const QString userName = user.value(QStringLiteral("userName")).toString();
    const QString nickName = user.value(QStringLiteral("nickName")).toString();
    const QString email = user.value(QStringLiteral("email")).toString();
    const QString phone = user.value(QStringLiteral("phone")).toString();
    const int sex = user.value(QStringLiteral("sex")).toInt();
    const QString avatar = user.value(QStringLiteral("avatar")).toString();
    const int role = user.value(QStringLiteral("role")).toInt();

    // 保存启动探测/兜底接口返回的完整用户字段，供后续模块复用（避免重复读取 storage 或二次探测）。
    _currentUser = user;
    QString name = nickName.trimmed();
    if (name.isEmpty()) {
        name = userName.trimmed();
    }
    if (name.isEmpty()) {
        name = tr("Logged in");
    }
    _displayName = name;
    UpdateLoginButtonState();
}

void CamDemo::OnTokenCleared(const QVariantMap& data)
{
    Q_UNUSED(data);
    // token 已清理：当前策略只需要释放探测视图即可，UI 状态已在 OnLogout() 中设置为未登录。
}