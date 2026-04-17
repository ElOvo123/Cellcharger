#include "mainwindow_controller.h"
#include <iostream>

MainWindowController::MainWindowController(QWidget *parent) : MainWindowView(parent)
{
    connect(this, &MainWindowView::helpClicked, this, &MainWindowController::onHelpWindow);
    connect(this, &MainWindowView::comsClicked, this, &MainWindowController::onComs);
    connect(this, &MainWindowView::consoleClicked, this, &MainWindowController::onConsole);
    connect(this, &MainWindowView::logClicked, this, &MainWindowController::onLog);
    connect(this, &MainWindowView::comsStatusClicked, this, &MainWindowController::onComsStatus);
}

void MainWindowController::onComs()
{
    MainWindowView::showComsWindow();
    backend.openComs();
}

void MainWindowController::onConsole()
{
    backend.openConsole();
    MainWindowView::openPanel(PanelType::Console);
}

void MainWindowController::onLog()
{
    backend.openLog();
    MainWindowView::openPanel(PanelType::Log);
}

void MainWindowController::onComsStatus()
{
    MainWindowView::openPanel(PanelType::ComsStatus);
}

void MainWindowController::onHelpWindow()
{
    std::cout << "Opening help window\n";
    MainWindowView::showHelpWindow();
}
