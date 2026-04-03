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
 * 嵌入 Web 的账户认证对话框（登录 / 注册 / 重置密码同页路由）。
 * 展示前宿主须已完成 QCef 全局初始化（见 main 中 QCefContext）。
 */
class AccountAuthDialog : public QDialog
{
    Q_OBJECT

public:
    /** authPageUrl 一般为 DesktopWeb::buildDesktopLoginUrl(frontendBase) */
    explicit AccountAuthDialog(QWidget* parent, const QUrl& authPageUrl);
    ~AccountAuthDialog() override;

signals:
    /** 认证成功（含 token 与 user），与 Web 侧约定字段一致 */
    void AuthSucceeded(const QVariantMap& payload);

private slots:
    void OnAddressChanged(const QString& url);
    void OnLoadEnd(int httpStatusCode);
    void OnInvokeMethod(const QString& method, const QVariantList& arguments);

private:
    bool IsTrustedInvokeSource() const;
    bool IsTrustedUiSource() const;
    void InjectDesktopBridgeScript();
    void HandleAuthSucceeded(const QVariantMap& payload);
    void SyncWindowTitleFromCurrentUrl();
    void UpdateUiFromUrl(const QUrl& url);

    QCefView* m_view { nullptr };
    QWidget* m_loadingCover { nullptr };
    bool m_mainFrameShown { false };
    QUrl m_currentUrl;
    QUrl m_authPageUrl;
};
