#pragma once

#include <QString>

class QWidget;

enum class PanelType
{
    Console,
    Log
};

class PanelFactory
{
public:
    static QWidget* createPanelWidget(PanelType type, QWidget *parent = nullptr);
    static QString panelTitle(PanelType type);
};