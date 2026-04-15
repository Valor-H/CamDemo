#pragma once

#include <QApplication>

class NApplication : public QApplication
{
public:
    NApplication(int& argc, char** argv);
    ~NApplication() override;

    void Initialize();

private:
    static void InitCefRuntime();
};