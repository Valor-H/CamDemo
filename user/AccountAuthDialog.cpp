#include "AccountAuthDialog.h"

#include "DesktopAuthBridge.h"
#include "LoginWebAuthHelpers.h"

#include <QUrl>
#include <QVariantList>
#include <QStackedLayout>
#include <QString>
#include <QWidget>
#include <QPalette>
#include <QSizePolicy>

#include <QCefView.h>

AccountAuthDialog::AccountAuthDialog(QWidget* parent, const QUrl& authPageUrl)
    : QDialog(parent)
    , m_authPageUrl(authPageUrl)
{
    setWindowTitle(tr("Login"));

    setFixedSize(450, 550);
    // 首帧前整窗透明，避免 CEF 未合成时出现黑底；就绪后一次性设为 1（无渐入）
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

    const QString startUrl = m_authPageUrl.toString();
    m_currentUrl = m_authPageUrl;

    // 不传 QCefSetting：全局已开 OSR，部分版本在 per-view setBackgroundColor 时会在 Qt 侧写坏内存
    m_view = new QCefView(startUrl, nullptr, this);

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
    // 只做标题同步，不做窗口 resize
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
        setWindowOpacity(1.0);
    }
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
