#include "LoginDialog.h"

#include "DesktopWeb.h"

#include <QUrl>
#include <QUrlQuery>
#include <QVariantList>
#include <QStackedLayout>
#include <QString>
#include <QWidget>
#include <QPalette>
#include <QSizePolicy>
#include <QColor>
#include <QPropertyAnimation>
#include <QEasingCurve>

#include <QCefSetting.h>
#include <QCefView.h>

namespace
{
QString ExtractAuthRoutePath(const QUrl& url)
{
    // 兼容 history 路由与 hash 路由：
    // - history: http://host/register => path="/register"
    // - hash:    http://host/?client=desktop#/register => path="/", fragment="/register"
    QString path = url.path().trimmed();
    if (!path.isEmpty() && path != QLatin1String("/")) {
        return path;
    }

    const QString frag = url.fragment(QUrl::FullyDecoded).trimmed();
    if (frag.startsWith(QLatin1Char('/'))) {
        const int q = frag.indexOf(QLatin1Char('?'));
        return q >= 0 ? frag.left(q) : frag;
    }
    return path;
}

enum class AuthRoute
{
    Login,
    Register,
    Reset,
    Unknown
};

AuthRoute RouteFromPath(const QString& path)
{
    QString p = path.trimmed();
    if (p.isEmpty() || p == QLatin1Char('/')) {
        return AuthRoute::Login;
    }
    // 兼容尾部斜杠：/login/ => /login
    if (p.size() > 1 && p.endsWith(QLatin1Char('/'))) {
        p.chop(1);
    }

    if (p == QStringLiteral("/login")) {
        return AuthRoute::Login;
    }
    if (p == QStringLiteral("/register")) {
        return AuthRoute::Register;
    }
    if (p == QStringLiteral("/reset-password")) {
        return AuthRoute::Reset;
    }

    return AuthRoute::Unknown;
}

QVariantMap SanitizeLoginPayload(const QVariantMap& input)
{
    QVariantMap sanitized;
    // 登录主链路：保留 token + user（对象），避免平铺字段导致协议膨胀。
    sanitized.insert(QStringLiteral("token"), input.value(QStringLiteral("token")).toString());

    const QVariantMap user = input.value(QStringLiteral("user")).toMap();
    if (!user.isEmpty()) {
        // 字段名与后端 Java 实体保持一致：uuid/userName/nickName/email/phone/sex/avatar/role
        QVariantMap u;
        u.insert(QStringLiteral("uuid"), user.value(QStringLiteral("uuid")).toString());
        u.insert(QStringLiteral("userName"), user.value(QStringLiteral("userName")).toString());
        u.insert(QStringLiteral("nickName"), user.value(QStringLiteral("nickName")).toString());
        u.insert(QStringLiteral("email"), user.value(QStringLiteral("email")).toString());
        u.insert(QStringLiteral("phone"), user.value(QStringLiteral("phone")).toString());
        u.insert(QStringLiteral("sex"), user.value(QStringLiteral("sex")).toInt());
        u.insert(QStringLiteral("avatar"), user.value(QStringLiteral("avatar")).toString());
        u.insert(QStringLiteral("role"), user.value(QStringLiteral("role")).toInt());
        sanitized.insert(QStringLiteral("user"), u);
    }
    return sanitized;
}
} // namespace

LoginDialog::LoginDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Login"));

    // 固定窗口尺寸：项目开发阶段不考虑兼容与自适应，去掉登录/注册/重置路由切换带来的 resize 逻辑。
    setFixedSize(450, 500);
    // 窗口级渐显：避免 CEF 首帧未就绪时露出黑底。
    setWindowOpacity(0.0);

    // 先给对话框一个明确的白底，避免 windowless CEF 首帧未渲染时露出黑底。
    setAutoFillBackground(true);
    {
        QPalette pal = palette();
        pal.setColor(QPalette::Window, Qt::white);
        setPalette(pal);
    }

    // 关键：使用堆叠布局实现“白底覆盖”。
    // 旧的 QVBoxLayout 会把 cover 与 view 垂直堆叠，某些情况下仍可能暴露出黑底区域。
    auto* layout = new QStackedLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setStackingMode(QStackedLayout::StackAll);

    m_loginUrl = BuildDesktopLoginUrl();
    m_currentUrl = QUrl(m_loginUrl);

    QCefSetting setting;
    // 尽量在创建浏览器前设置背景色，避免 windowless 首帧合成时默认黑底闪烁。
    // 注：QCefView/QCefSetting 的 API 以实际库版本为准；若此处编译失败，说明该版本不支持该接口。
    setting.setBackgroundColor(QColor(Qt::white));
    m_view = new QCefView(m_loginUrl, &setting, this);
    // 首帧黑底：默认显示白底 cover，CEF 在后台加载，待 loadEnd 后再切换到 view。

    // 白底占位层：覆盖在 CEF 视图上方，直到主框架 loadEnd。
    m_loadingCover = new QWidget(this);
    m_loadingCover->setAutoFillBackground(true);
    m_loadingCover->setPalette(palette());
    m_loadingCover->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    layout->addWidget(m_view);
    layout->addWidget(m_loadingCover);
    layout->setCurrentWidget(m_loadingCover);

    connect(m_view,
            &QCefView::addressChanged,
            this,
            [this](const QCefFrameId&, const QString& url) { OnAddressChanged(url); });
    connect(m_view,
            &QCefView::loadEnd,
            this,
            [this](const QCefBrowserId&, const QCefFrameId&, bool isMainFrame, int httpStatusCode) {
                if (isMainFrame) {
                    OnLoadEnd(httpStatusCode);
                }
            });
    connect(m_view,
            &QCefView::invokeMethod,
            this,
            [this](const QCefBrowserId&, const QCefFrameId&, const QString& method, const QVariantList& arguments) {
                OnInvokeMethod(method, arguments);
            });
    // CEF zoom level: factor ≈ 1.2^level (Chrome-style); ~1.5x ≈ log(1.5)/log(1.2)
    // m_view->setZoomLevel(std::log(1.5) / std::log(1.2));
}

LoginDialog::~LoginDialog()
{}

QString LoginDialog::BuildDesktopLoginUrl() const
{
    return DesktopWeb::BuildDesktopLoginUrl();
}

bool LoginDialog::IsTrustedInvokeSource() const
{
    if (!m_currentUrl.isValid()) {
        return false;
    }
    const QUrl expected(m_loginUrl);
    if (!expected.isValid()) {
        return false;
    }
    if (m_currentUrl.scheme() != expected.scheme() || m_currentUrl.host() != expected.host()
        || m_currentUrl.port() != expected.port()) {
        return false;
    }

    const QUrlQuery query(m_currentUrl);
    return query.queryItemValue(DesktopWeb::DesktopClientQueryKey()) == DesktopWeb::DesktopClientQueryValue();
}

bool LoginDialog::IsTrustedUiSource() const
{
    if (!m_currentUrl.isValid()) {
        return false;
    }
    const QUrl expected(m_loginUrl);
    if (!expected.isValid()) {
        return false;
    }
    // UI 同步只要求同源：避免前端某些跳转路径丢失 query 导致标题无法更新。
    return m_currentUrl.scheme() == expected.scheme() && m_currentUrl.host() == expected.host()
        && m_currentUrl.port() == expected.port();
}

void LoginDialog::InjectDesktopBridgeScript()
{
    // 桥接注入（单一职责）：
    // - 前端调用 window.__DESKTOP_QT__.onLoginSuccess(payload)
    // - 这里将其转发到 C++：CallBridge.invoke('Desktop.OnLoginSuccess', payload)
    // - 同时监听前端路由变化（hash/history），回传当前 href 用于同步标题（窗口尺寸保持固定）。
    //
    // 说明：/files 跳转拦截由前端桌面嵌入逻辑负责（减少重复与维护点）。
    static const QString kBridgeScript = QStringLiteral(R"JS(
      (function() {
        if (window.__DESKTOP_QT__ && window.__DESKTOP_QT__.__ready) {
          return;
        }
        function notifyRouteChanged() {
          try {
            if (window.CallBridge && typeof window.CallBridge.invoke === 'function') {
              window.CallBridge.invoke('Desktop.RouteChanged', { href: String(location.href || '') });
            }
          } catch (e) {
            // ignore
          }
        }
        // 监听 hash 路由与 history 路由（pushState/replaceState/popstate）。
        try {
          var _pushState = history.pushState;
          history.pushState = function() {
            var ret = _pushState.apply(this, arguments);
            notifyRouteChanged();
            return ret;
          };
          var _replaceState = history.replaceState;
          history.replaceState = function() {
            var ret = _replaceState.apply(this, arguments);
            notifyRouteChanged();
            return ret;
          };
        } catch (e) {
          // ignore
        }
        try {
          window.addEventListener('hashchange', notifyRouteChanged, true);
          window.addEventListener('popstate', notifyRouteChanged, true);
          window.addEventListener('DOMContentLoaded', notifyRouteChanged, true);
          window.addEventListener('load', notifyRouteChanged, true);
        } catch (e) {
          // ignore
        }

        window.__DESKTOP_QT__ = {
          __ready: true,
          onLoginSuccess: function(payload) {
            if (window.CallBridge && typeof window.CallBridge.invoke === 'function') {
              window.CallBridge.invoke('Desktop.OnLoginSuccess', payload || {});
              return true;
            }
            return false;
          }
        };
        // 初次注入后回传一次，确保标题与当前页一致。
        notifyRouteChanged();
      })();
    )JS");

    m_view->executeJavascript(QCefView::MainFrameID, kBridgeScript, m_loginUrl);
}

void LoginDialog::HandleLoginSuccess(const QVariantMap& payload)
{
    emit loginSucceeded(payload);
    accept();
}

void LoginDialog::OnAddressChanged(const QString& url)
{
    UpdateUiFromUrl(QUrl(url));
}

void LoginDialog::UpdateUiFromUrl(const QUrl& url)
{
    m_currentUrl = url;
    if (!IsTrustedUiSource()) {
        return;
    }
    // 中文注释：只做标题同步，不做窗口自适应/resize（窗口尺寸固定）。
    SyncWindowTitleFromCurrentUrl();
}

void LoginDialog::SyncWindowTitleFromCurrentUrl()
{
    const AuthRoute route = RouteFromPath(ExtractAuthRoutePath(m_currentUrl));
    switch (route) {
        case AuthRoute::Register:
            setWindowTitle(tr("Register"));
            return;
        case AuthRoute::Reset:
            setWindowTitle(tr("Reset password"));
            return;
        case AuthRoute::Login:
        case AuthRoute::Unknown:
        default:
            setWindowTitle(tr("Login"));
            return;
    }
}

void LoginDialog::OnLoadEnd(int httpStatusCode)
{
    Q_UNUSED(httpStatusCode);
    InjectDesktopBridgeScript();
    UpdateUiFromUrl(m_currentUrl);

    // 首帧黑底规避：只在第一次主框架 loadEnd 后展示 CEF 视图并移除白底占位。
    if (!m_mainFrameShown) {
        m_mainFrameShown = true;
        if (m_loadingCover) {
            // 使用 stacked layout 时不需要显式 show/hide view，只要移除 cover 即可。
            if (QStackedLayout* stacked = qobject_cast<QStackedLayout*>(layout())) {
                stacked->setCurrentWidget(m_view);
            }
            m_loadingCover->hide();
            m_loadingCover->deleteLater();
            m_loadingCover = nullptr;
        }

        // 首次 ready 后淡入显示窗口（测试：拉长时长便于观察），提升观感并避免黑底闪烁。
        if (windowOpacity() < 1.0) {
            auto* fadeIn = new QPropertyAnimation(this, "windowOpacity", this);
            fadeIn->setDuration(600);
            fadeIn->setEasingCurve(QEasingCurve::InOutCubic);
            fadeIn->setStartValue(windowOpacity());
            fadeIn->setEndValue(1.0);
            fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
        }
    }
}

void LoginDialog::OnInvokeMethod(const QString& method, const QVariantList& arguments)
{
    if (method == QStringLiteral("Desktop.OnLoginSuccess")) {
        if (!IsTrustedInvokeSource()) {
            return;
        }

        QVariantMap payload;
        if (!arguments.isEmpty()) {
            payload = SanitizeLoginPayload(arguments.front().toMap());
        }
        HandleLoginSuccess(payload);
        return;
    }

    if (method == QStringLiteral("Desktop.RouteChanged")) {
        if (arguments.isEmpty()) {
            return;
        }
        const QVariantMap data = arguments.front().toMap();
        const QString href = data.value(QStringLiteral("href")).toString().trimmed();
        if (href.isEmpty()) {
            return;
        }

        UpdateUiFromUrl(QUrl(href));
        return;
    }
}
