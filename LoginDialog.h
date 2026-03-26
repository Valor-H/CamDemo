#pragma once

#include <QDialog>

class QWebEngineView;

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget* parent = nullptr);
    ~LoginDialog() override;

private:
    QWebEngineView* m_view { nullptr };


};

