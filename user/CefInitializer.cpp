#include "CefInitializer.h"
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QCefConfig.h>
#include <QCefContext.h>
static QCefContext* g_cefContext = nullptr;
void CefInitializer::Init(QApplication* app, int argc, char* argv[]) {
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
    g_cefContext = new QCefContext(app, argc, argv, &config);
}
