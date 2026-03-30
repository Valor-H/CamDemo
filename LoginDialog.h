#pragma once

#include <QDialog>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QUrl>

class QCefView;
class QWidget;

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget* parent = nullptr);
    ~LoginDialog() override;

signals:
    void loginSucceeded(const QVariantMap& payload);

private slots:
    void OnAddressChanged(const QString& url);
    void OnLoadEnd(int httpStatusCode);
    void OnInvokeMethod(const QString& method, const QVariantList& arguments);

private:
    QString BuildDesktopLoginUrl() const;
    /** 严格校验：用于安全敏感桥接（如登录成功回调） */
    bool IsTrustedInvokeSource() const;
    /** 仅同源校验：用于 UI 同步（如标题），对丢 query 更容错 */
    bool IsTrustedUiSource() const;
    void InjectDesktopBridgeScript();
    void HandleLoginSuccess(const QVariantMap& payload);
    /** 根据当前可信 URL 路径同步对话框标题（登录 / 注册 / 重置密码） */
    void SyncWindowTitleFromCurrentUrl();
    /** 统一入口：更新当前 URL，并同步 UI（仅标题；窗口尺寸固定） */
    void UpdateUiFromUrl(const QUrl& url);

    QCefView* m_view { nullptr };
    QWidget* m_loadingCover { nullptr };
    bool m_mainFrameShown { false };
    QUrl m_currentUrl;
    QString m_loginUrl;
};
