#include "TitleBarUserChip.h"
#include "UserSession.h"

#include <QFontMetrics>
#include <QSizePolicy>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QMouseEvent>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QUrl>
#include <QSslSocket>

namespace
{
constexpr int kLabelFontPx = 12;
constexpr char kNoLoginAvatarRes[] = ":/CamDemo/resource/no-login-avatar.png";
const QUrl kBackendBaseUrl(QStringLiteral("http://localhost:8080/"));

/** 昵称区排版一致，仅颜色区分未登录 / 已登录 */
QString nameLabelStyleSheet(const QString& colorHex)
{
    return QString(QStringLiteral("QLabel { color: %1; font-size: %2px; font-weight: 500; }"))
        .arg(colorHex)
        .arg(kLabelFontPx);
}

QString elideToNameWidth(const QString& text, const QFont& font)
{
    return QFontMetrics(font).elidedText(text, Qt::ElideRight, TitleBarUserChip::kNameWidthPx);
}
} // namespace

TitleBarUserChip::TitleBarUserChip(QWidget* parent)
    : QWidget(parent)
    , _nam(new QNetworkAccessManager(this))
{
    setCursor(Qt::PointingHandCursor);
    setMinimumHeight(TitleBarUserChip::kAvatarSide);
    // 垂直用 Minimum，避免父级高度小于 sizeHint 时与 Fixed 策略冲突被压没
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    _avatarLabel = new QLabel(this);
    _avatarLabel->setFixedSize(TitleBarUserChip::kAvatarSide, TitleBarUserChip::kAvatarSide);
    _avatarLabel->setScaledContents(false);
    _avatarLabel->setAlignment(Qt::AlignCenter);

    _nameLabel = new QLabel(this);
    _nameLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    _nameLabel->setFixedWidth(TitleBarUserChip::kNameWidthPx);
    _nameLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(0, 0, 6, 0);
    lay->setSpacing(6);
    lay->addWidget(_avatarLabel);
    lay->addWidget(_nameLabel);

    connect(_nam, &QNetworkAccessManager::finished, this, &TitleBarUserChip::onAvatarDownloadFinished);
}

void TitleBarUserChip::relayoutInParent()
{
    updateGeometry();
    adjustSize();
    QWidget* w = parentWidget();
    for (int i = 0; w && i < 8; ++i, w = w->parentWidget()) {
        if (QLayout* lay = w->layout()) {
            lay->invalidate();
            lay->activate();
        }
        w->updateGeometry();
    }
}

void TitleBarUserChip::syncFromSession(const UserSession* session)
{
    abortAvatarRequest();
    _loggedIn = session && session->isAuthenticated();
    if (!_loggedIn) {
        applyLoggedOutAppearance();
        return;
    }
    applyLoggedInAppearance(session);
}

void TitleBarUserChip::applyLoggedOutAppearance()
{
    _fallbackNickName.clear();
    _fallbackUserName.clear();
    const QPixmap ph = loadAvatarRaster(kNoLoginAvatarRes, TitleBarUserChip::kAvatarSide * 2);
    _avatarLabel->setPixmap(makeCircularPlaceholder(ph));
    _avatarLabel->setStyleSheet(QStringLiteral("QLabel { background: transparent; border: none; }"));
    _nameLabel->setStyleSheet(nameLabelStyleSheet(QStringLiteral("#999999")));
    _nameLabel->ensurePolished();
    _nameLabel->setText(elideToNameWidth(tr("Not logged in"), _nameLabel->font()));
    _nameLabel->setToolTip(QString());
}

void TitleBarUserChip::applyLoggedInAppearance(const UserSession* session)
{
    const QVariantMap u = session->currentUser();
    const QString nick = u.value(QStringLiteral("nickName")).toString().trimmed();
    const QString userName = u.value(QStringLiteral("userName")).toString().trimmed();
    _fallbackNickName = nick;
    _fallbackUserName = userName;
    // 启动阶段用户信息由 Qt 直连后端拉取，昵称为空时不显示中间态文案。
    const QString fullName = nick;
    _nameLabel->setStyleSheet(nameLabelStyleSheet(QStringLiteral("#333333")));
    _nameLabel->ensurePolished();
    _nameLabel->setText(elideToNameWidth(fullName, _nameLabel->font()));
    _nameLabel->setToolTip(fullName.isEmpty() ? QString() : fullName);

    _avatarLabel->setStyleSheet(QStringLiteral("QLabel { background: transparent; border: none; }"));

    const QString raw = u.value(QStringLiteral("avatar")).toString().trimmed();
    if (raw.isEmpty()) {
        _avatarLabel->setPixmap(makeInitialAvatarWithRing(nick, userName));
        return;
    }

    const QUrl url = resolveAvatarUrl(raw);
    if (!url.isValid()) {
        _avatarLabel->setPixmap(makeInitialAvatarWithRing(nick, userName));
        return;
    }

    _avatarReply = _nam->get(QNetworkRequest(url));
}

QUrl TitleBarUserChip::resolveAvatarUrl(const QString& raw)
{
    if (raw.isEmpty()) {
        return {};
    }
    QUrl u = QUrl::fromUserInput(raw);
    if (u.isRelative()) {
        // 后端通常返回 /profile/... 这类相对路径，应以 Java 服务地址为基准解析。
        return kBackendBaseUrl.resolved(u);
    }

    // Windows 发布环境若未携带 OpenSSL，Qt 可能无法发起 HTTPS 请求；对 OSS 链接做协议降级兜底。
    if (u.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0
        && !QSslSocket::supportsSsl()) {
        u.setScheme(QStringLiteral("http"));
    }

    return u;
}

QString TitleBarUserChip::pickInitialChar(const QString& nickName, const QString& userName)
{
    const QString nick = nickName.trimmed();
    const QString user = userName.trimmed();
    QString seed = !nick.isEmpty() ? nick : user;
    if (seed.isEmpty()) {
        return QStringLiteral("?");
    }

    const QChar c = seed.at(0);
    if (c.isLetter() && c.unicode() <= 0x7F) {
        return QString(c.toUpper());
    }
    return QString(c);
}

QPixmap TitleBarUserChip::makeInitialAvatarWithRing(const QString& nickName, const QString& userName) const
{
    const int side = TitleBarUserChip::kAvatarSide;
    QPixmap out(side, side);
    out.fill(Qt::transparent);

    QPainter painter(&out);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white);
    painter.drawEllipse(0, 0, side, side);

    painter.setBrush(QColor(QStringLiteral("#999999")));
    painter.drawEllipse(2, 2, side - 4, side - 4);

    QFont f = font();
    f.setBold(true);
    f.setPixelSize(11);
    painter.setFont(f);
    painter.setPen(QColor(Qt::white));
    painter.drawText(QRect(2, 2, side - 4, side - 4), Qt::AlignCenter, pickInitialChar(nickName, userName));
    return out;
}

QPixmap TitleBarUserChip::loadAvatarRaster(const char* resourcePath, int side)
{
    QPixmap loaded;
    if (!loaded.load(QString::fromUtf8(resourcePath))) {
        QPixmap px(side, side);
        px.fill(QColor(QStringLiteral("#E0E0E0")));
        return px;
    }
    return loaded.scaled(side, side, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
}

QPixmap TitleBarUserChip::makeCircularPlaceholder(const QPixmap& source) const
{
    const int side = TitleBarUserChip::kAvatarSide;
    QPixmap out(side, side);
    out.fill(Qt::transparent);
    QPainter painter(&out);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPainterPath clip;
    clip.addEllipse(0, 0, side, side);
    painter.setClipPath(clip);
    painter.fillRect(0, 0, side, side, QColor(QStringLiteral("#E0E0E0")));
    if (!source.isNull()) {
        const QPixmap scaled = source.scaled(side, side, Qt::KeepAspectRatioByExpanding,
                                             Qt::SmoothTransformation);
        const int x = (side - scaled.width()) / 2;
        const int y = (side - scaled.height()) / 2;
        painter.drawPixmap(x, y, scaled);
    }
    return out;
}

QPixmap TitleBarUserChip::makeCircularAvatarWithRing(const QPixmap& source) const
{
    const int side = TitleBarUserChip::kAvatarSide;
    QPixmap out(side, side);
    out.fill(Qt::transparent);
    QPainter painter(&out);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white);
    painter.drawEllipse(0, 0, side, side);
    if (source.isNull()) {
        return out;
    }
    const QPixmap scaled = source.scaled(side - 4, side - 4, Qt::KeepAspectRatioByExpanding,
                                         Qt::SmoothTransformation);
    QPainterPath inner;
    inner.addEllipse(2, 2, side - 4, side - 4);
    painter.setClipPath(inner);
    const int x = 2 + (side - 4 - scaled.width()) / 2;
    const int y = 2 + (side - 4 - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);
    return out;
}

void TitleBarUserChip::abortAvatarRequest()
{
    if (_avatarReply) {
        disconnect(_avatarReply, nullptr, this, nullptr);
        _avatarReply->abort();
        _avatarReply->deleteLater();
        _avatarReply = nullptr;
    }
}

void TitleBarUserChip::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (_loggedIn) {
            emit accountMenuRequested();
        } else {
            emit loginRequested();
        }
    }
    QWidget::mouseReleaseEvent(event);
}

void TitleBarUserChip::onAvatarDownloadFinished(QNetworkReply* reply)
{
    if (!reply) {
        return;
    }
    if (reply != _avatarReply) {
        reply->deleteLater();
        return;
    }
    _avatarReply = nullptr;

    QPixmap loaded;
    if (reply->error() == QNetworkReply::NoError) {
        loaded.loadFromData(reply->readAll());
    }
    reply->deleteLater();

    if (loaded.isNull()) {
        _avatarLabel->setPixmap(makeInitialAvatarWithRing(_fallbackNickName, _fallbackUserName));
    } else {
        _avatarLabel->setPixmap(makeCircularAvatarWithRing(loaded));
    }
    relayoutInParent();
    QTimer::singleShot(0, this, [this]() { relayoutInParent(); });
}
