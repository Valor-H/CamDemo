#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

/** 桌面端登录会话：与 CEF localStorage / 登录对话框 payload 字段约定一致 */
class UserSession final : public QObject
{
    Q_OBJECT

public:
    explicit UserSession(QObject* parent = nullptr);

    bool isAuthenticated() const { return _authenticated; }
    QString authToken() const { return _authToken; }
    QVariantMap currentUser() const { return _currentUser; }

    void applyFromLoginPayload(const QVariantMap& payload);
    void applyFromProbe(const QVariantMap& data);
    void logout();

signals:
    void authStateChanged(bool authenticated);
    /** 用户信息或 token 更新（含登录成功），供顶栏刷新头像等 */
    void userProfileChanged();

private:
    void setAuthenticatedState(bool on);

    bool _authenticated { false };
    QString _authToken;
    QVariantMap _currentUser;
};
