#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

#include <atomic>
#include <functional>
#include <memory>

/**
 * 基于 libhv 的异步 HTTP 客户端。
 *
 * 请求在 libhv I/O 线程中发起；回调通过 Qt 事件队列（QueuedConnection）
 * 投递回 Qt 主线程，因此回调中可以安全访问 Qt/UI 对象。
 *
 * cancelAll() 使所有进行中请求的回调成为空操作，
 * 包括已进入 Qt 事件队列但尚未执行的回调。
 */
class AuthHttpClient : public QObject
{
    Q_OBJECT

public:
    /** 回调收到的统一响应描述 */
    struct Response
    {
        bool        networkOk { false };  ///< false = 连接/超时等网络层失败
        int         httpStatus { 0 };     ///< HTTP 状态码（200、401 等）
        int         bizCode { -1 };       ///< AjaxResult.code（解析失败时为 -1）
        QString     bizMsg;               ///< AjaxResult.msg
        QVariantMap data;                 ///< AjaxResult.data 解析为 QVariantMap
    };

    using Callback = std::function<void(const Response&)>;

    explicit AuthHttpClient(const QString& baseUrl, QObject* parent = nullptr);
    ~AuthHttpClient() override;

    /**
     * 异步 POST baseUrl + path，携带 Bearer token，超时 timeoutSec 秒。
     * callback 保证在 Qt 主线程执行。
     */
    void post(const QString& path,
              const QString& bearerToken,
              int            timeoutSec,
              Callback       callback);

    /**
     * 使所有进行中请求的回调成为空操作（线程安全，可在任意线程调用）。
     * 调用后先前注册的所有 callback 均不会被执行。
     */
    void cancelAll();

    QString baseUrl() const { return _baseUrl; }
    void    setBaseUrl(const QString& url) { _baseUrl = url; }

private:
    QString                                _baseUrl;
    std::shared_ptr<std::atomic<quint64>>  _cancelEpoch;
};
