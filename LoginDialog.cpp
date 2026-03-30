#include "LoginDialog.h"

#include "DesktopWeb.h"

#include <QUrl>
#include <QUrlQuery>
#include <QVariantList>
#include <QVBoxLayout>

#include <QCefSetting.h>
#include <QCefView.h>

namespace
{
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
    setFixedSize(450, 650);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_loginUrl = BuildDesktopLoginUrl();
    m_currentUrl = QUrl(m_loginUrl);

    QCefSetting setting;
    m_view = new QCefView(m_loginUrl, &setting, this);
    layout->addWidget(m_view);

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

void LoginDialog::InjectDesktopBridgeScript()
{
    // 桥接注入（单一职责）：
    // - 前端调用 window.__CAMDEMO_QT__.onLoginSuccess(payload)
    // - 这里将其转发到 C++：CallBridge.invoke('CamDemo.OnLoginSuccess', payload)
    //
    // 说明：/files 跳转拦截由前端桌面嵌入逻辑负责（减少重复与维护点）。
    static const QString kBridgeScript = QStringLiteral(R"JS(
      (function() {
        if (window.__CAMDEMO_QT__ && window.__CAMDEMO_QT__.__ready) {
          return;
        }
        window.__CAMDEMO_QT__ = {
          __ready: true,
          onLoginSuccess: function(payload) {
            if (window.CallBridge && typeof window.CallBridge.invoke === 'function') {
              window.CallBridge.invoke('CamDemo.OnLoginSuccess', payload || {});
              return true;
            }
            return false;
          }
        };
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
    m_currentUrl = QUrl(url);
    if (!IsTrustedInvokeSource()) {
        return;
    }
    SyncWindowTitleFromCurrentUrl();
}

void LoginDialog::SyncWindowTitleFromCurrentUrl()
{
    const QString path = m_currentUrl.path();
    // 根路径或重定向前：与 /login 同属登录流程
    if (path.isEmpty() || path == QLatin1Char('/')) {
        setWindowTitle(tr("Login"));
        return;
    }
    if (path == QStringLiteral("/login")) {
        setWindowTitle(tr("Login"));
        return;
    }
    if (path == QStringLiteral("/register")) {
        setWindowTitle(tr("Register"));
        return;
    }
    if (path == QStringLiteral("/reset-password")) {
        setWindowTitle(tr("Reset password"));
        return;
    }
    setWindowTitle(tr("Login"));
}

void LoginDialog::OnLoadEnd(int httpStatusCode)
{
    Q_UNUSED(httpStatusCode);
    if (!IsTrustedInvokeSource()) {
        return;
    }
    InjectDesktopBridgeScript();
}

void LoginDialog::OnInvokeMethod(const QString& method, const QVariantList& arguments)
{
    if (method != QStringLiteral("CamDemo.OnLoginSuccess")) {
        return;
    }
    if (!IsTrustedInvokeSource()) {
        return;
    }

    QVariantMap payload;
    if (!arguments.isEmpty()) {
        payload = SanitizeLoginPayload(arguments.front().toMap());
    }
    HandleLoginSuccess(payload);
}
