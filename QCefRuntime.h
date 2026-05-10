#pragma once

#include <QCefConfig.h>

#include <QPointer>

class QCefContext;

class QCefRuntime
{
public:
    static QCefRuntime& Instance();

    void Initialize();

private:
    QCefRuntime() = default;
    ~QCefRuntime() = default;

    QCefRuntime(const QCefRuntime&) = delete;
    QCefRuntime& operator=(const QCefRuntime&) = delete;

    void InitConfig();

    QCefConfig m_config;
    QPointer<QCefContext> m_context;
};