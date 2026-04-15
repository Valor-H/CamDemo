#include "CamDemo.h"
#include <QCoreApplication>
#include <QDir>
#include <QString>
#include <QStandardPaths>
#include <QTranslator>
#include <QtWidgets/QApplication>
#include <QColor>

#include <QCefConfig.h>
#include <QCefContext.h>

namespace
{
void InitCefConfig(QCefConfig& config)
{
    config.setLogLevel(QCefConfig::LOGSEVERITY_DEFAULT);
    config.setBridgeObjectName(QStringLiteral("CallBridge"));
    config.setBuiltinSchemeName(QStringLiteral("CefView"));
    config.setRemoteDebuggingPort(0);
    config.setWindowlessRenderingEnabled(true);
    config.setStandaloneMessageLoopEnabled(true);
    config.setSandboxDisabled(true);
    config.setBackgroundColor(QColor(Qt::white));

    const QString appDir = QCoreApplication::applicationDirPath();
    const QString cefBundle = QDir(appDir).filePath(QStringLiteral("CefView"));
    config.setResourceDirectoryPath(QDir(cefBundle).filePath(QStringLiteral("Resources")));
    config.setLocalesDirectoryPath(QDir(cefBundle).filePath(QStringLiteral("locales")));
    config.setCachePath(
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QStringLiteral("/CamDemo/cef"));
}
}

int main(int argc, char *argv[])
{
#if (QT_VERSION <= QT_VERSION_CHECK(6, 0, 0))
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
#endif

    QApplication app(argc, argv);

    QCefConfig config;
    InitCefConfig(config);

    QCefContext cefContext(&app, argc, argv, &config);

    QTranslator appTr;
    const QString qmDir = QCoreApplication::applicationDirPath() + QStringLiteral("/translations");
    if (appTr.load(QStringLiteral("CamDemo_zh_CN"), qmDir)) {
        app.installTranslator(&appTr);
    }

    CamDemo window;
    window.showMaximized();
    return app.exec();
}
