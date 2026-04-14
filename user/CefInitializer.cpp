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
    config.setWindowlessRenderingEnabled(true);
    config.setStandaloneMessageLoopEnabled(true);
    config.setSandboxDisabled(true);
    // config.setBackgroundColor(QColor(Qt::white)); // 暂时禁用，QCefView 版本可能不兼容
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString cefBundle = QDir(appDir).filePath(QStringLiteral("CefView"));
    // resources.pak 等文件直接在 CefView/ 下，不在 CefView/Resources/ 下
    config.setResourceDirectoryPath(cefBundle);
    config.setLocalesDirectoryPath(QDir(cefBundle).filePath(QStringLiteral("locales")));
#endif
    config.setCachePath(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QStringLiteral("/CamDemo/cef"));
    g_cefContext = new QCefContext(app, argc, argv, &config);
}
