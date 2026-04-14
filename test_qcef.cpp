#include <QApplication>
#include <QCefConfig.h>
#include <QCefContext.h>
#include <QDebug>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    QCefConfig config;
    qDebug() << "Testing QCefConfig...";
    
    // 测试基本设置
    config.setLogLevel(QCefConfig::LOGSEVERITY_VERBOSE);
    qDebug() << "setLogLevel OK";
    
    config.setSandboxDisabled(true);
    qDebug() << "setSandboxDisabled OK";
    
    // 这行如果崩溃，说明 ABI 不兼容
    config.setBackgroundColor(QColor(Qt::white));
    qDebug() << "setBackgroundColor OK";
    
    QColor c = config.backgroundColor().value<QColor>();
    qDebug() << "backgroundColor read OK:" << c;
    
    return 0;
}
