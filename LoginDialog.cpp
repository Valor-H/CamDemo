#include "LoginDialog.h"

#include <QVBoxLayout>
#include <QWebEngineView>
#include <QUrl>

LoginDialog::LoginDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Login"));
    setFixedSize(800, 1050);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_view = new QWebEngineView(this);
    layout->addWidget(m_view);
    m_view->setZoomFactor(1.5);
    // 测试网址：后续替换成你的登录页
    m_view->load(QUrl(QStringLiteral("http://localhost:5173/")));
}

LoginDialog::~LoginDialog()
{}

