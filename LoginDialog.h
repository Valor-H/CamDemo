#pragma once

#include <QDialog>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QUrl>

class QCefView;

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
    bool IsTrustedInvokeSource() const;
    void InjectDesktopBridgeScript();
    void HandleLoginSuccess(const QVariantMap& payload);

    QCefView* m_view { nullptr };
    QUrl m_currentUrl;
    QString m_loginUrl;
};
