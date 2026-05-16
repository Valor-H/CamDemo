#pragma once

#include "cloud_server_global.h"

#include <QUrl>
#include <QWidget>

class SARibbonToolButton;
class QNetworkAccessManager;
class QNetworkReply;

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN
class UserSession;

class CLOUD_SERVER_EXPORT TitleBarUserChip final : public QWidget
{
    Q_OBJECT

public:
    /** 定义头像按钮边长。 */
    static constexpr int kAvatarButtonSide = 20;
    /** 定义头像图标边长。 */
    static constexpr int kAvatarIconSide = 18;
    /** 构造标题栏用户信息组件。 */
    explicit TitleBarUserChip(QWidget* parent, const QUrl& apiBaseUrl);
    /** 根据会话信息同步显示状态。 */
    void SyncFromSession(const UserSession* session);
    /** 重新在父部件中布局自身。 */
    void RelayoutInParent();

signals:
    /** 请求显示登录入口。 */
    void loginRequested();
    /** 请求显示账号菜单。 */
    void accountMenuRequested();

private slots:
    /** 处理头像下载完成事件。 */
    void OnAvatarDownloadFinished(QNetworkReply* reply);

private:
    /** 中止当前头像下载请求。 */
    void AbortAvatarRequest();
    /** 应用默认头像显示。 */
    void ApplyDefaultAvatar();
    /** 应用已登录状态外观。 */
    void ApplyLoggedInAppearance(const UserSession* session);
    /** 生成带圆环的默认头像。 */
    QPixmap MakeInitialAvatarWithRing(const QString& nickName, const QString& userName) const;
    /** 选取头像显示首字符。 */
    static QString PickInitialChar(const QString& nickName, const QString& userName);
    /** 生成圆形头像图像。 */
    QPixmap MakeCircularAvatar(const QPixmap& source) const;
    /** 解析头像资源地址。 */
    QUrl ResolveAvatarUrl(const QString& raw) const;
    /** 从资源中加载头像位图。 */
    static QPixmap LoadAvatarRaster(const char* resourcePath, int side);
    /** 保存接口基础地址。 */
    QUrl _apiBaseUrl;
    /** 保存头像按钮实例。 */
    SARibbonToolButton* _avatarButton { nullptr };
    /** 保存网络访问管理器。 */
    QNetworkAccessManager* _nam { nullptr };
    /** 保存当前头像下载回复对象。 */
    QNetworkReply* _avatarReply { nullptr };
    /** 标识当前是否处于登录状态。 */
    bool _loggedIn { false };
    /** 保存兜底昵称。 */
    QString _fallbackNickName;
    /** 保存兜底用户名。 */
    QString _fallbackUserName;
};

QJ_NAMESPACE_FIT_CLOUD_SERVER_END

