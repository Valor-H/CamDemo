#include "AuthHttpClient.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>

// libhv: 静态链接时需要 HV_STATICLIB（由 CMake hv_static target 自动注入）
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
    // 在派发时快照当前纪元；将 shared_ptr 传入 lambda 使其在 this 析构后仍安全。
    const quint64 myEpoch  = _cancelEpoch->load(std::memory_order_relaxed);
    auto          epochRef = _cancelEpoch;

    auto req = std::make_shared<HttpRequest>();
    req->method  = HTTP_POST;
    req->url     = (_baseUrl + path).toStdString();
    req->timeout = timeoutSec;
    req->headers["Authorization"] = "Bearer " + bearerToken.toStdString();
    req->headers["Content-Type"]  = "application/json";
    req->body = "{}";

    // requests::async 在 libhv 全局 I/O 线程中发起请求并回调。
    requests::async(req,
        [epochRef, myEpoch, cb = std::move(callback)](const HttpResponsePtr& resp) {
            // ── 此处在 libhv I/O 线程 ──
            // 通过 Qt 事件队列把工作投递回主线程；lambda 按值捕获，无悬挂引用。
            QMetaObject::invokeMethod(
                QCoreApplication::instance(),
                [epochRef, myEpoch, resp, cb]() {
                    // ── 此处在 Qt 主线程 ──
                    if (epochRef->load(std::memory_order_relaxed) != myEpoch) {
                        return;  // 请求已被 cancelAll() 取消
                    }

                    Response result;
                    if (!resp) {
                        // 网络层失败（连接拒绝、超时等）
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
