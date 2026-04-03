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
#include <QAbstractAnimation>
#include <QPropertyAnimation>
#include <QEasingCurve>

#include <QCefSetting.h>
#include <QCefView.h>

namespace
{
QString ExtractAuthRoutePath(const QUrl& url)
{
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
    sanitized.insert(QStringLiteral("token"), input.value(QStringLiteral("token")).toString());

    const QVariantMap user = input.value(QStringLiteral("user")).toMap();
    if (!user.isEmpty()) {
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

LoginDialog::LoginDialog(QWidget* parent, const QUrl& desktopLoginPageUrl)
    : QDialog(parent)
    , m_loginPageUrl(desktopLoginPageUrl)
{
    setWindowTitle(tr("Login"));

    setFixedSize(450, 550);
    setWindowOpacity(0.0);

    setAutoFillBackground(true);
    {
        QPalette pal = palette();
        pal.setColor(QPalette::Window, Qt::white);
        setPalette(pal);
    }

    auto* layout = new QStackedLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setStackingMode(QStackedLayout::StackAll);

    const QString startUrl = m_loginPageUrl.toString();
    m_currentUrl = m_loginPageUrl;

    QCefSetting setting;
    setting.setBackgroundColor(QColor(Qt::white));
    m_view = new QCefView(startUrl, &setting, this);

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
}

LoginDialog::~LoginDialog()
{}

bool LoginDialog::IsTrustedInvokeSource() const
{
    if (!m_currentUrl.isValid()) {
        return false;
    }
    if (!m_loginPageUrl.isValid()) {
        return false;
    }
    if (m_currentUrl.scheme() != m_loginPageUrl.scheme() || m_currentUrl.host() != m_loginPageUrl.host()
        || m_currentUrl.port() != m_loginPageUrl.port()) {
        return false;
    }

    const QUrlQuery query(m_currentUrl);
    return query.queryItemValue(DesktopWeb::desktopClientQueryKey())
        == DesktopWeb::desktopClientQueryValue();
}

bool LoginDialog::IsTrustedUiSource() const
{
    if (!m_currentUrl.isValid()) {
        return false;
    }
    if (!m_loginPageUrl.isValid()) {
        return false;
    }
    return m_currentUrl.scheme() == m_loginPageUrl.scheme() && m_currentUrl.host() == m_loginPageUrl.host()
        && m_currentUrl.port() == m_loginPageUrl.port();
}

void LoginDialog::InjectDesktopBridgeScript()
{
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
          }
        }
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
        }
        try {
          window.addEventListener('hashchange', notifyRouteChanged, true);
          window.addEventListener('popstate', notifyRouteChanged, true);
          window.addEventListener('DOMContentLoaded', notifyRouteChanged, true);
          window.addEventListener('load', notifyRouteChanged, true);
        } catch (e) {
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
        notifyRouteChanged();
      })();
    )JS");

    m_view->executeJavascript(QCefView::MainFrameID, kBridgeScript, m_loginPageUrl.toString());
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
    // 只做标题同步，不做窗口 resize
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

    if (!m_mainFrameShown) {
        m_mainFrameShown = true;
        if (m_loadingCover) {
            if (QStackedLayout* stacked = qobject_cast<QStackedLayout*>(layout())) {
                stacked->setCurrentWidget(m_view);
            }
            m_loadingCover->hide();
            m_loadingCover->deleteLater();
            m_loadingCover = nullptr;
        }

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
