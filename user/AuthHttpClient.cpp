#include "AuthHttpClient.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>

#include <hv/requests.h>

AuthHttpClient::AuthHttpClient(const QString& baseUrl, QObject* parent)
    : QObject(parent)
    , _baseUrl(baseUrl)
    , _cancelEpoch(std::make_shared<std::atomic<quint64>>(0))
{}

AuthHttpClient::~AuthHttpClient()
{
    cancelAll();
}

void AuthHttpClient::post(
    const QString& path,
    const QString& bearerToken,
    int            timeoutSec,
    Callback       callback)
{
    const quint64 myEpoch  = _cancelEpoch->load(std::memory_order_relaxed);
    auto          epochRef = _cancelEpoch;

    auto req = std::make_shared<HttpRequest>();
    req->method  = HTTP_POST;
    req->url     = (_baseUrl + path).toStdString();
    req->timeout = timeoutSec;
    req->headers["Authorization"] = "Bearer " + bearerToken.toStdString();
    req->headers["Content-Type"]  = "application/json";
    req->body = "{}";

    requests::async(req,
        [epochRef, myEpoch, cb = std::move(callback)](const HttpResponsePtr& resp) {
            QMetaObject::invokeMethod(
                QCoreApplication::instance(),
                [epochRef, myEpoch, resp, cb]() {
                    if (epochRef->load(std::memory_order_relaxed) != myEpoch) {
                        return;
                    }

                    Response result;
                    if (!resp) {
                        result.networkOk = false;
                        cb(result);
                        return;
                    }

                    result.networkOk  = true;
                    result.httpStatus = resp->status_code;

                    const QByteArray body = QByteArray::fromStdString(resp->body);
                    const QJsonDocument doc = QJsonDocument::fromJson(body);
                    if (doc.isObject()) {
                        const QJsonObject root = doc.object();
                        result.bizCode = root.value(QStringLiteral("code")).toInt(-1);
                        result.bizMsg  = root.value(QStringLiteral("msg")).toString();
                        const QJsonObject dataObj =
                            root.value(QStringLiteral("data")).toObject();
                        if (!dataObj.isEmpty()) {
                            result.data = dataObj.toVariantMap();
                        }
                    }

                    cb(result);
                },
                Qt::QueuedConnection);
        });
}

void AuthHttpClient::cancelAll()
{
    _cancelEpoch->fetch_add(1, std::memory_order_relaxed);
}
