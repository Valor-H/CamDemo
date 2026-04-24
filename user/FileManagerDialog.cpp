#include "FileManagerDialog.h"

#include "AuthHttpClient.h"
#include "LocalFilesBridge.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QPalette>
#include <QPointer>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QVariantMap>
#include <QVariantList>
#include <QVBoxLayout>

#include <QJsonDocument>
#include <QJsonObject>

#include <QtConcurrent/QtConcurrentRun>

#include <QCefSetting.h>
#include <QCefView.h>

namespace
{
constexpr int kQjpFileType = 11;
constexpr int kRecentFileDownloadTimeoutSec = 60;

#ifndef CAMDEMO_RECENT_FILE_CACHE_DIR
#define CAMDEMO_RECENT_FILE_CACHE_DIR "D:/cachePath/"
#endif

QString apiBaseStringForClient(const QUrl& url)
{
    QString baseUrl = url.toString(QUrl::RemoveQuery | QUrl::RemoveFragment);
    while (baseUrl.endsWith(QLatin1Char('/'))) {
        baseUrl.chop(1);
    }
    return baseUrl;
}

QString sanitizeFileSegment(QString value)
{
    value = value.trimmed();
    if (value.isEmpty()) {
        return QStringLiteral("recent-file");
    }

    value.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9._-]")), QStringLiteral("_"));
    return value;
}
}

FileManagerDialog::FileManagerDialog(QWidget* parent, qianjizn::user::UserAuthService* authService, const QUrl& pageUrl)
    : QDialog(parent)
    , m_authService(authService)
    , m_pageUrl(pageUrl)
{
    setWindowTitle(tr("文件管理"));
    resize(1200, 760);
    setAutoFillBackground(true);

    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    QCefSetting setting;
    setting.setBackgroundColor(QColor(Qt::white));

    m_view = new QCefView(pageUrl.toString(), &setting, this);
    m_view->setAutoFillBackground(true);
    m_view->setPalette(palette());
    layout->addWidget(m_view);

    m_snapshotWatcher = new QFutureWatcher<LocalFilesSnapshot::Result>(this);

    connect(m_view,
            &QCefView::loadEnd,
            this,
            [this](const QCefBrowserId&, const QCefFrameId&, bool isMainFrame, int) {
                if (isMainFrame) {
                    OnLoadEnd();
                }
            });
    connect(m_view,
            &QCefView::invokeMethod,
            this,
            [this](const QCefBrowserId&, const QCefFrameId&, const QString& method, const QVariantList& arguments) {
                OnInvokeMethod(method, arguments);
            });
    connect(m_snapshotWatcher, &QFutureWatcher<LocalFilesSnapshot::Result>::finished, this, [this]() {
        PushLocalFilesSnapshot(m_snapshotWatcher->result());
    });
}

FileManagerDialog::~FileManagerDialog() = default;

void FileManagerDialog::InjectDesktopBridgeScript()
{
    if (!m_view) {
        return;
    }
    m_view->executeJavascript(QCefView::MainFrameID, LocalFilesBridge::BridgeInjectScript(), m_pageUrl.toString());
}

void FileManagerDialog::OnLoadEnd()
{
    InjectDesktopBridgeScript();
}

void FileManagerDialog::OnInvokeMethod(const QString& method, const QVariantList& arguments)
{
    if (method == LocalFilesBridge::MethodRequestLocalFiles()) {
        RequestLocalFilesSnapshot();
        return;
    }

    if (method == LocalFilesBridge::MethodRequestOpenRecentFile()) {
        const QVariantMap payload = arguments.isEmpty() ? QVariantMap {} : arguments.first().toMap();
        HandleOpenRecentFileRequest(payload);
    }
}

void FileManagerDialog::RequestLocalFilesSnapshot()
{
    if (!m_snapshotWatcher || m_snapshotWatcher->isRunning()) {
        return;
    }

    m_snapshotWatcher->setFuture(QtConcurrent::run([]() {
        return LocalFilesSnapshot::ScanRoot();
    }));
}

void FileManagerDialog::PushLocalFilesSnapshot(const LocalFilesSnapshot::Result& result)
{
    if (!m_view) {
        return;
    }

    QJsonObject payload {
        { QStringLiteral("rootPath"), result.rootPath },
    };
    QString script;

    if (result.ok) {
        payload.insert(QStringLiteral("files"), result.files);
        const QString json = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
        script = QStringLiteral(
                     "(function(){"
                     "if(window.__DESKTOP_QT__&&typeof window.__DESKTOP_QT__.onLocalFilesSnapshot==='function'){"
                     "window.__DESKTOP_QT__.onLocalFilesSnapshot(%1);"
                     "}"
                     "})();")
                     .arg(json);
    } else {
        payload.insert(QStringLiteral("code"), result.errorCode);
        payload.insert(QStringLiteral("message"), result.errorMessage);
        const QString json = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
        script = QStringLiteral(
                     "(function(){"
                     "if(window.__DESKTOP_QT__&&typeof window.__DESKTOP_QT__.onLocalFilesError==='function'){"
                     "window.__DESKTOP_QT__.onLocalFilesError(%1);"
                     "}"
                     "})();")
                     .arg(json);
    }

    m_view->executeJavascript(QCefView::MainFrameID, script, m_pageUrl.toString());
}

void FileManagerDialog::HandleOpenRecentFileRequest(const QVariantMap& payload)
{
    if (!IsDesktopRecentRequest(payload)) {
        NotifyOpenRecentFileError(QStringLiteral("仅支持桌面端最近文件中的 QJP 打开"), QStringLiteral("recent_open_rejected"));
        return;
    }

    const QString source = payload.value(QStringLiteral("source")).toString().trimmed();
    if (source == QStringLiteral("local")) {
        if (OpenRecentLocalFile(payload)) {
            return;
        }
        NotifyOpenRecentFileError(QStringLiteral("本地 QJP 文件无效或无法打开"), QStringLiteral("recent_local_invalid"));
        return;
    }

    if (source == QStringLiteral("cloud")) {
        OpenRecentCloudFile(payload);
        return;
    }

    NotifyOpenRecentFileError(QStringLiteral("未知的最近文件来源"), QStringLiteral("recent_source_invalid"));
}

bool FileManagerDialog::OpenRecentLocalFile(const QVariantMap& payload)
{
    const QString filePath = ResolveRecentLocalPath(payload);
    const QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()
        || fileInfo.suffix().compare(QStringLiteral("qjp"), Qt::CaseInsensitive) != 0) {
        return false;
    }

    emit OpenFileRequested(fileInfo.absoluteFilePath());
    NotifyOpenRecentFileSuccess(fileInfo.absoluteFilePath());
    return true;
}

void FileManagerDialog::OpenRecentCloudFile(const QVariantMap& payload)
{
    if (!m_authService) {
        NotifyOpenRecentFileError(QStringLiteral("桌面认证服务不可用"), QStringLiteral("recent_open_no_auth_service"));
        return;
    }

    const QVariantMap currentUser = m_authService->Session()->CurrentUser();
    const QString userUuid = currentUser.value(QStringLiteral("uuid")).toString().trimmed();
    const QString token = m_authService->Session()->AuthToken().trimmed();
    const QString fileUuid = payload.value(QStringLiteral("fileUuid")).toString().trimmed();

    if (token.isEmpty() || userUuid.isEmpty()) {
        NotifyOpenRecentFileError(QStringLiteral("当前登录态缺少 token 或用户 uuid"), QStringLiteral("recent_open_no_auth"));
        return;
    }
    if (fileUuid.isEmpty()) {
        NotifyOpenRecentFileError(QStringLiteral("云端最近文件缺少 fileUuid"), QStringLiteral("recent_open_no_uuid"));
        return;
    }

    const QString cacheFilePath = BuildCacheFilePath(payload);
    const QFileInfo cacheInfo(cacheFilePath);
    QDir cacheDir(cacheInfo.dir());
    if (!cacheDir.exists() && !cacheDir.mkpath(QStringLiteral("."))) {
        NotifyOpenRecentFileError(QStringLiteral("无法创建缓存目录"), QStringLiteral("recent_open_cache_dir_failed"));
        return;
    }

    AuthHttpClient* httpClient = EnsureFileHttpClient();
    if (!httpClient) {
        NotifyOpenRecentFileError(QStringLiteral("桌面下载客户端初始化失败"), QStringLiteral("recent_open_http_unavailable"));
        return;
    }

    QJsonObject requestBody {
        { QStringLiteral("fileUuid"), fileUuid },
        { QStringLiteral("UserUuid"), userUuid },
    };

    QPointer<FileManagerDialog> self(this);
    httpClient->PostJsonToFile(
        QStringLiteral("/api/file/getFile"),
        token,
        QJsonDocument(requestBody).toJson(QJsonDocument::Compact),
        cacheFilePath,
        kRecentFileDownloadTimeoutSec,
        [self](const AuthHttpClient::DownloadResponse& response) {
            if (!self) {
                return;
            }

            if (!response.networkOk) {
                QFile::remove(response.targetFilePath);
                self->NotifyOpenRecentFileError(QStringLiteral("云文件下载失败：%1").arg(response.errorMessage), QStringLiteral("recent_open_cloud_network"));
                return;
            }

            if (!response.writeOk) {
                QFile::remove(response.targetFilePath);
                self->NotifyOpenRecentFileError(QStringLiteral("云文件缓存失败：%1").arg(response.errorMessage), QStringLiteral("recent_open_cloud_cache"));
                return;
            }

            emit self->OpenFileRequested(response.targetFilePath);
            self->NotifyOpenRecentFileSuccess(response.targetFilePath);
        });
}

QString FileManagerDialog::ResolveRecentLocalPath(const QVariantMap& payload) const
{
    const QString path = payload.value(QStringLiteral("path")).toString().trimmed();
    if (!path.isEmpty()) {
        return QDir::fromNativeSeparators(path);
    }

    const QString fileUuid = payload.value(QStringLiteral("fileUuid")).toString().trimmed();
    if (fileUuid.startsWith(QStringLiteral("local::"))) {
        return QDir::fromNativeSeparators(fileUuid.mid(QStringLiteral("local::").size()));
    }
    return QString();
}

QString FileManagerDialog::BuildCacheFilePath(const QVariantMap& payload) const
{
    const QString fileName = payload.value(QStringLiteral("fileName")).toString().trimmed();
    QString candidate = sanitizeFileSegment(fileName);
    if (!candidate.endsWith(QStringLiteral(".qjp"), Qt::CaseInsensitive)) {
        const QString fileUuid = sanitizeFileSegment(payload.value(QStringLiteral("fileUuid")).toString());
        candidate = fileUuid.isEmpty() ? candidate : fileUuid;
        if (!candidate.endsWith(QStringLiteral(".qjp"), Qt::CaseInsensitive)) {
            candidate += QStringLiteral(".qjp");
        }
    }

    return QDir::fromNativeSeparators(QStringLiteral(CAMDEMO_RECENT_FILE_CACHE_DIR))
        + QLatin1Char('/') + candidate;
}

bool FileManagerDialog::IsDesktopRecentRequest(const QVariantMap& payload) const
{
    const QString path = m_pageUrl.path().trimmed();
    const bool isRecentRoute = path == QStringLiteral("/local-files") || path == QStringLiteral("/recent-files");
    const bool fromRecent = payload.value(QStringLiteral("fromRecent")).toBool();
    const int fileType = payload.value(QStringLiteral("fileType")).toInt();
    const QString source = payload.value(QStringLiteral("source")).toString().trimmed();

    return isRecentRoute
        && fromRecent
        && fileType == kQjpFileType
        && (source == QStringLiteral("local") || source == QStringLiteral("cloud"));
}

void FileManagerDialog::NotifyOpenRecentFileError(const QString& message, const QString& code)
{
    if (!m_view) {
        return;
    }

    const QJsonObject payload {
        { QStringLiteral("code"), code },
        { QStringLiteral("message"), message },
    };
    const QString json = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    const QString script = QStringLiteral(
        "(function(){"
        "if(window.__DESKTOP_QT__&&typeof window.__DESKTOP_QT__.onOpenRecentFileError==='function'){"
        "window.__DESKTOP_QT__.onOpenRecentFileError(%1);"
        "}"
        "})();")
        .arg(json);
    m_view->executeJavascript(QCefView::MainFrameID, script, m_pageUrl.toString());
}

void FileManagerDialog::NotifyOpenRecentFileSuccess(const QString& openedPath)
{
    if (!m_view) {
        return;
    }

    const QJsonObject payload {
        { QStringLiteral("openedPath"), openedPath },
    };
    const QString json = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    const QString script = QStringLiteral(
        "(function(){"
        "if(window.__DESKTOP_QT__&&typeof window.__DESKTOP_QT__.onOpenRecentFileSuccess==='function'){"
        "window.__DESKTOP_QT__.onOpenRecentFileSuccess(%1);"
        "}"
        "})();")
        .arg(json);
    m_view->executeJavascript(QCefView::MainFrameID, script, m_pageUrl.toString());
}

AuthHttpClient* FileManagerDialog::EnsureFileHttpClient()
{
    if (m_fileHttpClient) {
        return m_fileHttpClient;
    }

    if (!m_authService) {
        return nullptr;
    }

    m_fileHttpClient = new AuthHttpClient(apiBaseStringForClient(m_authService->ApiBaseUrl()), this);
    return m_fileHttpClient;
}
