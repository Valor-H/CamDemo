#include "NApplication.h"
#include "QCefRuntime.h"

NApplication::NApplication(int& argc, char** argv)
    : QApplication(argc, argv)
    , m_argc(argc)
    , m_argv(argv)
{}

NApplication::~NApplication() = default;

void NApplication::Initialize()
{
    if (QCefRuntime::Instance().IsInitialized()) {
        return;
    }

    InitCefConfig();
}

void NApplication::InitCefConfig()
{
    QCefRuntime::Instance().Initialize(this, m_argc, m_argv);
}