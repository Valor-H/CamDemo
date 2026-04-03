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

    bool IsAuthenticated() const { return _authenticated; }
    QString AuthToken() const { return _authToken; }
    QVariantMap CurrentUser() const { return _currentUser; }

    void ApplyFromLoginPayload(const QVariantMap& payload);
    void ApplyFromProbe(const QVariantMap& data);
    void Logout();

signals:
    void AuthStateChanged(bool authenticated);
    /** 用户信息或 token 更新（含登录成功），供顶栏刷新头像等 */
    void UserProfileChanged();

private:
    void SetAuthenticatedState(bool on);

    bool _authenticated { false };
    QString _authToken;
    QVariantMap _currentUser;
};
