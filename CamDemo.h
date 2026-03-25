#pragma once

#include "SARibbonMainWindow.h"
#include "ui_CamDemo.h"

class CamDemo : public SARibbonMainWindow
{
    Q_OBJECT

public:
    CamDemo(QWidget *parent = nullptr);
    ~CamDemo();

private:
    void buildRibbon();
    void setupWindowUserAvatar();
    Ui::CamDemoClass ui;
};

