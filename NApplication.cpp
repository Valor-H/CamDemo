#include "NApplication.h"
#include "QCefRuntime.h"

NApplication::NApplication(int& argc, char** argv)
    : QApplication(argc, argv)
{}

NApplication::~NApplication() = default;

void NApplication::Initialize()
{
    InitCefRuntime();
}

void NApplication::InitCefRuntime()
{
    QCefRuntime::Instance().Initialize();
}