#include "mainwindow_controller.h"

MainWindowController::MainWindowController(QWidget *parent)
    : MainWindowView(parent) {
    
    connect(this, &MainWindowView::comsClicked, this, &MainWindowController::onComs);
    connect(this, &MainWindowView::consoleClicked, this, &MainWindowController::onConsole);

}

void MainWindowController::onComs() {
    backend.openComs();
}

void MainWindowController::onConsole() {
    backend.openConsole();
}