#pragma once

#include "cloud_server_global.h"
#include <QCefConfig.h>
#include <QPointer>

class QCefContext;

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

class CLOUD_SERVER_EXPORT QCefRuntime
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

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
