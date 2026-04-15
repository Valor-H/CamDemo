#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

#include <atomic>
#include <functional>
#include <memory>

class AuthHttpClient : public QObject
{
    Q_OBJECT

public:
    struct Response
    {
        bool        networkOk { false };
        int         httpStatus { 0 };
        int         bizCode { -1 };
        QString     bizMsg;
        QVariantMap data;
    };

    using Callback = std::function<void(const Response&)>;

    explicit AuthHttpClient(const QString& baseUrl, QObject* parent = nullptr);
    ~AuthHttpClient() override;

    void Post(const QString& path,
              const QString& bearerToken,
              int            timeoutSec,
              Callback       callback);

    void CancelAll();

    QString BaseUrl() const { return _baseUrl; }
    void    SetBaseUrl(const QString& url) { _baseUrl = url; }

private:
    QString                                _baseUrl;
    std::shared_ptr<std::atomic<quint64>>  _cancelEpoch;
};
