#pragma once

#include <QUrl>
#include <QWidget>

class QLabel;
class QMouseEvent;
class QNetworkAccessManager;
class QNetworkReply;
class UserSession;

/** 标题栏右侧：小尺寸头像 + 昵称，点击未登录弹登录 / 已登录出账户菜单 */
class TitleBarUserChip final : public QWidget
{
    Q_OBJECT

public:
    /** 头像边长；标题栏对齐高度勿小于此值 */
    static constexpr int kAvatarSide = 24;
    /** 昵称文字区固定宽度，超出由 QFontMetrics 省略 */
    static constexpr int kNameWidthPx = 64;

    explicit TitleBarUserChip(QWidget* parent, const QUrl& apiBaseUrl);
    void syncFromSession(const UserSession* session);
    /** 向上失效布局，适配 SARibbon 标题条不随子控件 sizeHint 自动重算的问题 */
    void relayoutInParent();

signals:
    void loginRequested();
    void accountMenuRequested();

protected:
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void onAvatarDownloadFinished(QNetworkReply* reply);

private:
    void abortAvatarRequest();
    void applyLoggedOutAppearance();
    void applyLoggedInAppearance(const UserSession* session);
    QPixmap makeInitialAvatarWithRing(const QString& nickName, const QString& userName) const;
    static QString pickInitialChar(const QString& nickName, const QString& userName);
    /** 已登录/中性场景：内圈图 + 2px 白边 */
    QPixmap makeCircularAvatarWithRing(const QPixmap& source) const;
    QUrl resolveAvatarUrl(const QString& raw) const;
    static QPixmap loadAvatarRaster(const char* resourcePath, int side);

    QUrl _apiBaseUrl;

    QLabel* _avatarLabel { nullptr };
    QLabel* _nameLabel { nullptr };
    QNetworkAccessManager* _nam { nullptr };
    QNetworkReply* _avatarReply { nullptr };
    bool _loggedIn { false };
    QString _fallbackNickName;
    QString _fallbackUserName;
};

