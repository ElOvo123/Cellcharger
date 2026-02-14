#pragma once

#include "mainwindow_view.h"
#include "interface_backend.h"

class MainWindowController : public MainWindowView {
    Q_OBJECT

public:
    explicit MainWindowController(QWidget *parent = nullptr);

private slots:
    void onHelpWindow();
    void onComs();
    void onConsole();

private:
    InterfaceBackend backend;
};
