#include "panel_factory.h"
#include "console_widget.h"
#include "log_widget.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

QWidget* PanelFactory::createPanelWidget(PanelType type, QWidget *parent)
{
    switch (type)
    {
        case PanelType::Console:
            return new ConsoleWidget(parent);
        case PanelType::Log:
            return new LogWidget(parent);
    }

    return nullptr;
}

QString PanelFactory::panelTitle(PanelType type)
{
    switch (type)
    {
        case PanelType::Console:
            return "Console";
        case PanelType::Log:
            return "Log";
    }

    return "Panel";
}