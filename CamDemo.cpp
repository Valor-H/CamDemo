#include "CamDemo.h"
#include <QIcon>

CamDemo::CamDemo(QWidget* parent)
    : NMainWindow(parent)
{
    ui.setupUi(this);
    setWindowTitle(tr("CamDemo"));
    setWindowIcon(QIcon(QStringLiteral(":/CamDemo/resource/logo.ico")));
    setMinimumSize(1000, 800);
    InitializeMainWindowShell();
}

CamDemo::~CamDemo() = default;
