#include "CamDemo.h"
#include "SARibbonBar.h"
#include "SARibbonCategory.h"
#include "SARibbonPanel.h"
#include "SARibbonSystemButtonBar.h"

#include <QAbstractButton>
#include <QIcon>
#include <QSizePolicy>
#include <QTimer>
#include <QToolButton>
#include <QWidget>

CamDemo::CamDemo(QWidget *parent)
    : SARibbonMainWindow(parent)
{
    ui.setupUi(this);
    setWindowTitle(tr("CamDemo"));
    setWindowIcon(QIcon(QStringLiteral(":/CamDemo/resource/logo.png")));

    buildRibbon();
    setupWindowUserAvatar();
}

CamDemo::~CamDemo()
{}

void CamDemo::buildRibbon()
{
    SARibbonBar* ribbon = ribbonBar();
    if (!ribbon) {
        return;
    }
    ribbon->setRibbonStyle(SARibbonBar::RibbonStyleLooseThreeRow);

    SARibbonCategory* categoryMain = ribbon->addCategoryPage(tr("Main"));
    SARibbonPanel* panelFile = categoryMain->addPanel(tr("File"));

    QAction* actionNew  = new QAction(QIcon(), tr("New"),  this);
    QAction* actionOpen = new QAction(QIcon(), tr("Open"), this);
    QAction* actionSave = new QAction(QIcon(), tr("Save"), this);

    panelFile->addLargeAction(actionNew);
    panelFile->addLargeAction(actionOpen);
    panelFile->addLargeAction(actionSave);
}

void CamDemo::setupWindowUserAvatar()
{
    SARibbonSystemButtonBar* bar = windowButtonBar();
    if (!bar) {
        return;
    }
    QWidget* spacer = new QWidget(bar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    bar->addWidget(spacer);

    QToolButton* avatarBtn = new QToolButton(bar);
    avatarBtn->setObjectName(QStringLiteral("SAUserAvatarButton"));
    avatarBtn->setAutoRaise(true);
    avatarBtn->setIcon(QIcon(QStringLiteral(":/CamDemo/resource/avatar.png")));
    avatarBtn->setText(tr("Not logged in"));
    avatarBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    avatarBtn->setFocusPolicy(Qt::NoFocus);
    bar->addWidget(avatarBtn);

    QTimer::singleShot(0, this, [bar, avatarBtn]() {
        int rowH = bar->windowTitleHeight();
        if (QAbstractButton* minBtn = bar->minimizeButton()) {
            const int mh = minBtn->height();
            if (mh > 0) {
                rowH = mh;
            }
        }
        if (rowH > 0) {
            avatarBtn->setFixedHeight(rowH);
        }
        const int iconDim = qMax(16, rowH - 8);
        avatarBtn->setIconSize(QSize(iconDim, iconDim));
        avatarBtn->setMinimumWidth(avatarBtn->sizeHint().width());
    });
}
