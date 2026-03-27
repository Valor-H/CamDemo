#include "LoginDialog.h"

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
    // 登录主链路：明确保留 token，供主窗口直接建立登录态。
    sanitized.insert(QStringLiteral("token"), input.value(QStringLiteral("token")).toString());
    if (input.contains(QStringLiteral("nickName"))) {
        sanitized.insert(QStringLiteral("nickName"), input.value(QStringLiteral("nickName")).toString());
    }
    if (input.contains(QStringLiteral("userName"))) {
        sanitized.insert(QStringLiteral("userName"), input.value(QStringLiteral("userName")).toString());
    }
    if (input.contains(QStringLiteral("username"))) {
        sanitized.insert(QStringLiteral("username"), input.value(QStringLiteral("username")).toString());
    }
    if (input.contains(QStringLiteral("userId"))) {
        sanitized.insert(QStringLiteral("userId"), input.value(QStringLiteral("userId")).toString());
    }
    if (input.contains(QStringLiteral("loginAt"))) {
        sanitized.insert(QStringLiteral("loginAt"), input.value(QStringLiteral("loginAt")).toString());
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
    // Keep default URL compatible with existing local front-end dev workflow.
    QUrl url(QStringLiteral("http://localhost:5173/"));
    QUrlQuery query(url);
    query.addQueryItem(QStringLiteral("client"), QStringLiteral("desktop"));
    url.setQuery(query);
    return url.toString();
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
    return query.queryItemValue(QStringLiteral("client")) == QStringLiteral("desktop");
}

void LoginDialog::InjectDesktopBridgeScript()
{
    // 兜底防回归：桌面登录窗内禁止跳转到 /files，避免登录成功后页面闪跳。
    static const QString kBridgeScript = QStringLiteral(R"JS(
      (function() {
        if (window.__CAMDEMO_QT__ && window.__CAMDEMO_QT__.__ready) {
          return;
        }
        const BLOCK_PATH = '/files';
        const getPath = function(input) {
          try {
            const u = new URL(input, window.location.origin);
            return u.pathname;
          } catch (e) {
            return '';
          }
        };
        const isBlockedPath = function(path) {
          return path === BLOCK_PATH || path.indexOf(BLOCK_PATH + '/') === 0;
        };
        const fallbackToLogin = function() {
          const current = window.location.pathname;
          if (isBlockedPath(current)) {
            const search = window.location.search || '';
            history.replaceState(history.state, '', '/login' + search);
          }
        };
        const wrapHistoryMethod = function(name) {
          const raw = history[name];
          history[name] = function(state, title, url) {
            if (typeof url === 'string' && isBlockedPath(getPath(url))) {
              return null;
            }
            return raw.apply(this, arguments);
          };
        };
        wrapHistoryMethod('pushState');
        wrapHistoryMethod('replaceState');
        window.addEventListener('popstate', fallbackToLogin);
        window.addEventListener('hashchange', fallbackToLogin);
        fallbackToLogin();
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
