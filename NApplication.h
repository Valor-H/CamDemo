#pragma once

#include <QApplication>

class NApplication : public QApplication
{
public:
    NApplication(int& argc, char** argv);
    ~NApplication() override;

    void Initialize();

private:
    void InitCefConfig();

    int m_argc;
    char** m_argv;
};