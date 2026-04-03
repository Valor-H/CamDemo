#pragma once

#include <QDialog>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QUrl>

class QCefView;
class QWidget;

/**
 * 嵌入 Web 的登录/注册/重置密码对话框。
 * 展示前宿主须已完成 QCef 全局初始化（见 main 中 QCefContext）。
 */
class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    /** desktopLoginPageUrl 一般为 DesktopWeb::buildDesktopLoginUrl(frontendBase) */
    explicit LoginDialog(QWidget* parent, const QUrl& desktopLoginPageUrl);
    ~LoginDialog() override;

signals:
    void loginSucceeded(const QVariantMap& payload);

private slots:
    void OnAddressChanged(const QString& url);
    void OnLoadEnd(int httpStatusCode);
    void OnInvokeMethod(const QString& method, const QVariantList& arguments);

private:
    bool IsTrustedInvokeSource() const;
    bool IsTrustedUiSource() const;
    void InjectDesktopBridgeScript();
    void HandleLoginSuccess(const QVariantMap& payload);
    void SyncWindowTitleFromCurrentUrl();
    void UpdateUiFromUrl(const QUrl& url);

    QCefView* m_view { nullptr };
    QWidget* m_loadingCover { nullptr };
    bool m_mainFrameShown { false };
    QUrl m_currentUrl;
    QUrl m_loginPageUrl;
};
