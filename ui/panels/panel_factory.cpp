#include "panel_factory.h"
#include "console_widget.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

QWidget* PanelFactory::createPanelWidget(PanelType type, QWidget *parent)
{
    switch (type)
    {
        case PanelType::Console:
            return new ConsoleWidget(parent);
    }

    return nullptr;
}

QString PanelFactory::panelTitle(PanelType type)
{
    switch (type)
    {
        case PanelType::Console:
            return "Console";
    }

    return "Panel";
}