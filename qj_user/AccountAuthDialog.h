#pragma once

#include <QDialog>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QUrl>

class QCefView;
class QWidget;

class AccountAuthDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AccountAuthDialog(QWidget* parent, const QUrl& authPageUrl);
    ~AccountAuthDialog() override;

signals:
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
    QUrl m_currentUrl;
    QUrl m_authPageUrl;
};
