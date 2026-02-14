#include "mainwindow_view.h"
#include "ui_mainwindow.h"
#include "help_dialog.h"

MainWindowView::MainWindowView(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow) {

    ui->setupUi(this);
    
    connect(ui->menuHelp, &QMenu::aboutToShow, this, &MainWindowView::showHelpWindow);
    connect(ui->comsButton, &QToolButton::clicked, this, &MainWindowView::comsClicked);
    connect(ui->consoleButton, &QToolButton::clicked, this, &MainWindowView::consoleClicked);
}

MainWindowView::~MainWindowView() {
    delete ui;
}

void MainWindowView::centerWindow(QWidget* child)
{
    if (!child) return;

    child->adjustSize();

    QPoint parentCenter = this->geometry().center();
    child->move(parentCenter.x() - child->width() / 2, parentCenter.y() - child->height() / 2);
}

void MainWindowView::showHelpWindow(){
    ui->menuHelp->hide();

    HelpDialog dialog(this);
    dialog.exec();
    centerWindow(&dialog);
}
