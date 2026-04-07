#pragma once

#include <QUrl>
#include <QWidget>

class SARibbonToolButton;
class QNetworkAccessManager;
class QNetworkReply;
class UserSession;

/** 标题栏右侧：SARibbonToolButton 承载头像，悬停样式与 Ribbon 导航小按钮一致 */
class TitleBarUserChip final : public QWidget
{
    Q_OBJECT

public:
    /** 头像边长；标题栏对齐高度勿小于此值 */
    static constexpr int kAvatarSide = 24;

    explicit TitleBarUserChip(QWidget* parent, const QUrl& apiBaseUrl);
    void SyncFromSession(const UserSession* session);
    /** 向上失效布局，适配 SARibbon 标题条不随子控件 sizeHint 自动重算的问题 */
    void RelayoutInParent();

signals:
    void loginRequested();
    void accountMenuRequested();

private slots:
    void OnAvatarDownloadFinished(QNetworkReply* reply);

private:
    void AbortAvatarRequest();
    void ApplyLoggedOutAppearance();
    void ApplyLoggedInAppearance(const UserSession* session);
    QPixmap MakeInitialAvatarWithRing(const QString& nickName, const QString& userName) const;
    static QString PickInitialChar(const QString& nickName, const QString& userName);
    /** 已登录/中性场景：圆形裁剪位图 */
    QPixmap MakeCircularAvatarWithRing(const QPixmap& source) const;
    QUrl ResolveAvatarUrl(const QString& raw) const;
    static QPixmap LoadAvatarRaster(const char* resourcePath, int side);

    QUrl _apiBaseUrl;

    SARibbonToolButton* _avatarButton { nullptr };
    QNetworkAccessManager* _nam { nullptr };
    QNetworkReply* _avatarReply { nullptr };
    bool _loggedIn { false };
    QString _fallbackNickName;
    QString _fallbackUserName;
};

