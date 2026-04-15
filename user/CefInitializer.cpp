#include "CefInitializer.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QStringList>
#include <QCefConfig.h>
#include <QCefContext.h>

#include <vector>
static QCefContext* g_cefContext = nullptr;

QJ_NAMESPACE_FIT_QJ_USER_BEGIN
void InitCef(QApplication* app) {
    if (g_cefContext) {
        return;
    }
    QCefConfig config;
    config.setLogLevel(QCefConfig::LOGSEVERITY_DEFAULT);
    config.setBridgeObjectName(QStringLiteral("CallBridge"));
    config.setBuiltinSchemeName(QStringLiteral("CefView"));
    config.setRemoteDebuggingPort(0);
    config.setWindowlessRenderingEnabled(false);
    config.setStandaloneMessageLoopEnabled(true);
    config.setSandboxDisabled(true);
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString cefBundle = QDir(appDir).filePath(QStringLiteral("CefView"));
    config.setResourceDirectoryPath(cefBundle);
    config.setLocalesDirectoryPath(QDir(cefBundle).filePath(QStringLiteral("locales")));
#endif
    config.setCachePath(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QStringLiteral("/CamDemo/cef"));

    const QStringList args = QCoreApplication::arguments();
    std::vector<QByteArray> utf8;
    utf8.reserve(args.size());
    for (const QString& s : args) {
        utf8.emplace_back(s.toUtf8());
    }
    std::vector<char*> argv;
    argv.reserve(utf8.size());
    for (QByteArray& a : utf8) {
        argv.push_back(a.data());
    }
    g_cefContext = new QCefContext(app, static_cast<int>(argv.size()), argv.data(), &config);
}
QJ_NAMESPACE_FIT_QJ_USER_END
