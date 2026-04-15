#include "AccountAuthDialog.h"

#include "DesktopAuthBridge.h"
#include "LoginWebAuthHelpers.h"

#include <QUrl>
#include <QVariantList>
#include <QVBoxLayout>
#include <QString>
#include <QWidget>
#include <QPalette>

#include <QCefSetting.h>
#include <QCefView.h>

AccountAuthDialog::AccountAuthDialog(QWidget* parent, const QUrl& authPageUrl)
    : QDialog(parent)
    , m_authPageUrl(authPageUrl)
{
    setWindowTitle(tr("Login"));

    setFixedSize(450, 550);

    setAutoFillBackground(true);
    {
        QPalette pal = palette();
        pal.setColor(QPalette::Window, Qt::white);
        setPalette(pal);
    }

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    const QString startUrl = m_authPageUrl.toString();
    m_currentUrl = m_authPageUrl;

    QCefSetting setting;
    setting.setBackgroundColor(QColor(Qt::white));
    m_view = new QCefView(startUrl, &setting, this);
    m_view->setAutoFillBackground(true);
    m_view->setPalette(palette());

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
}

AccountAuthDialog::~AccountAuthDialog()
{}

bool AccountAuthDialog::IsTrustedInvokeSource() const
{
    return LoginWebAuth::IsTrustedInvokeSource(m_currentUrl, m_authPageUrl);
}

bool AccountAuthDialog::IsTrustedUiSource() const
{
    return LoginWebAuth::IsTrustedUiSource(m_currentUrl, m_authPageUrl);
}

void AccountAuthDialog::InjectDesktopBridgeScript()
{
    if (!m_view) {
        return;
    }
    m_view->executeJavascript(QCefView::MainFrameID, DesktopAuthBridge::BridgeInjectScript(),
                              m_authPageUrl.toString());
}

void AccountAuthDialog::HandleAuthSucceeded(const QVariantMap& payload)
{
    emit AuthSucceeded(payload);
    accept();
}

void AccountAuthDialog::OnAddressChanged(const QString& url)
{
    UpdateUiFromUrl(QUrl(url));
}

void AccountAuthDialog::UpdateUiFromUrl(const QUrl& url)
{
    m_currentUrl = url;
    if (!IsTrustedUiSource()) {
        return;
    }
    SyncWindowTitleFromCurrentUrl();
}

void AccountAuthDialog::SyncWindowTitleFromCurrentUrl()
{
    using LoginWebAuth::AuthRoute;
    const AuthRoute route = LoginWebAuth::RouteFromPath(LoginWebAuth::ExtractAuthRoutePath(m_currentUrl));
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

void AccountAuthDialog::OnLoadEnd(int httpStatusCode)
{
    Q_UNUSED(httpStatusCode);
    InjectDesktopBridgeScript();
    UpdateUiFromUrl(m_currentUrl);
}

void AccountAuthDialog::OnInvokeMethod(const QString& method, const QVariantList& arguments)
{
    if (method == DesktopAuthBridge::MethodOnLoginSuccess()) {
        if (!IsTrustedInvokeSource()) {
            return;
        }

        QVariantMap payload;
        if (!arguments.isEmpty()) {
            payload = LoginWebAuth::SanitizeLoginPayload(arguments.front().toMap());
        }
        HandleAuthSucceeded(payload);
        return;
    }

    if (method == DesktopAuthBridge::MethodRouteChanged()) {
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
