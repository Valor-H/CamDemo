#pragma once

#include "user_global.h"

#include "LocalFilesSnapshot.h"
#include "UserAuthService.h"

#include <QByteArray>
#include <QDialog>
#include <QFutureWatcher>
#include <QUrl>
#include <QVariant>

class QString;

class QCefView;
class QWidget;
class AuthHttpClient;

class USER_EXPORT FileManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FileManagerDialog(QWidget* parent, qianjizn::user::UserAuthService* authService, const QUrl& pageUrl);
    ~FileManagerDialog() override;

signals:
    void OpenFileRequested(const QString& filePath);

private:
    void InjectDesktopBridgeScript();
    void OnLoadEnd();
    void OnInvokeMethod(const QString& method, const QVariantList& arguments);
    void RequestLocalFilesSnapshot();
    void PushLocalFilesSnapshot(const LocalFilesSnapshot::Result& result);
    void HandleOpenRecentFileRequest(const QVariantMap& payload);
    bool OpenRecentLocalFile(const QVariantMap& payload);
    void OpenRecentCloudFile(const QVariantMap& payload);
    QString ResolveRecentLocalPath(const QVariantMap& payload) const;
    QString BuildCacheFilePath(const QVariantMap& payload) const;
    bool IsDesktopRecentRequest(const QVariantMap& payload) const;
    void NotifyOpenRecentFileError(const QString& message, const QString& code = QStringLiteral("recent_open_failed"));
    void NotifyOpenRecentFileSuccess(const QString& openedPath);
    AuthHttpClient* EnsureFileHttpClient();

    QUrl m_pageUrl;
    QCefView* m_view { nullptr };
    QFutureWatcher<LocalFilesSnapshot::Result>* m_snapshotWatcher { nullptr };
    AuthHttpClient* m_fileHttpClient { nullptr };
    qianjizn::user::UserAuthService* m_authService { nullptr };
};
