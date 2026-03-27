#include "LoginDialog.h"

#include <cmath>

#include <QVBoxLayout>

#include <QCefSetting.h>
#include <QCefView.h>

LoginDialog::LoginDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Login"));
    setFixedSize(600, 800);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    QCefSetting setting;
    const QString url = QStringLiteral("http://localhost:5173/");
    m_view = new QCefView(url, &setting, this);
    layout->addWidget(m_view);

    // CEF zoom level: factor ≈ 1.2^level (Chrome-style); ~1.5x ≈ log(1.5)/log(1.2)
    // m_view->setZoomLevel(std::log(1.5) / std::log(1.2));
}

LoginDialog::~LoginDialog()
{}
