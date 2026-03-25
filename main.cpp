#include "CamDemo.h"
#include <QCoreApplication>
#include <QString>
#include <QTranslator>
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QTranslator appTr;
    const QString qmDir = QCoreApplication::applicationDirPath() + QStringLiteral("/translations");
    if (appTr.load(QStringLiteral("CamDemo_zh_CN"), qmDir)) {
        app.installTranslator(&appTr);
    }

    CamDemo window;
    window.showMaximized();
    return app.exec();
}
