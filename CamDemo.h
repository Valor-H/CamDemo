#pragma once

#include "NMainWindow.h"
#include "ui_CamDemo.h"

class CamDemo : public NMainWindow
{
    Q_OBJECT

public:
    CamDemo(QWidget* parent = nullptr);
    ~CamDemo() override;

    Ui::CamDemoClass ui;
};
