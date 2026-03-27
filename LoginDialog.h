#pragma once

#include <QDialog>

class QCefView;

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget* parent = nullptr);
    ~LoginDialog() override;

private:
    QCefView* m_view { nullptr };
};
