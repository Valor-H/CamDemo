#include "CamDemo.h"
#include "CefInitializer.h"
#include <QCoreApplication>
#include <QString>
#include <QTranslator>
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
#if (QT_VERSION <= QT_VERSION_CHECK(6, 0, 0))
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
#endif

    QApplication app(argc, argv);

    qianjizn::qj_user::InitCef(&app);

    QTranslator appTr;
    const QString qmDir = QCoreApplication::applicationDirPath() + QStringLiteral("/translations");
    if (appTr.load(QStringLiteral("CamDemo_zh_CN"), qmDir)) {
        app.installTranslator(&appTr);
    }

    CamDemo window;
    window.showMaximized();
    return app.exec();
}
