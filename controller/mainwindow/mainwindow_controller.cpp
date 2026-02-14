#include "mainwindow_controller.h"
#include <iostream>

MainWindowController::MainWindowController(QWidget *parent)
    : MainWindowView(parent) {
    
    connect(this, &MainWindowView::helpClicked, this, &MainWindowController::onHelpWindow);
    connect(this, &MainWindowView::comsClicked, this, &MainWindowController::onComs);
    connect(this, &MainWindowView::consoleClicked, this, &MainWindowController::onConsole);

}

void MainWindowController::onComs() {
    backend.openComs();
}

void MainWindowController::onConsole() {
    backend.openConsole();
}

void MainWindowController::onHelpWindow() {
    std::cout << "Opening help window\n"; 
    MainWindowView::showHelpWindow();
}