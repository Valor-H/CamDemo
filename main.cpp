#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("CamDemo");
    a.setApplicationVersion("1.0");
    a.setOrganizationName("CamDemo");

    MainWindow w;
    w.show();

    return a.exec();
}
