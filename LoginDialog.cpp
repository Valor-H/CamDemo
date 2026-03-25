#include "LoginDialog.h"

#include <QVBoxLayout>
#include <QWebEngineView>
#include <QUrl>

LoginDialog::LoginDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Login"));
    resize(900, 600);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_view = new QWebEngineView(this);
    layout->addWidget(m_view);

    // 测试网址：后续替换成你的登录页
    m_view->load(QUrl(QStringLiteral("https://example.com")));
}

LoginDialog::~LoginDialog()
{}

