#include "mainwindow_view.h"
#include "ui_mainwindow.h"
#include "help_dialog.h"
#include "console_widget.h"
#include "logger_backend.h"
#include "coms.h"
#include "coms_controller.h"

#include <QSplitter>
#include <QMenu>
#include <QList>

MainWindowView::MainWindowView(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow), m_consoleSplitter(new QSplitter(Qt::Horizontal, this))
{
    ui->setupUi(this);

    setCentralWidget(m_consoleSplitter);
    m_consoleSplitter->setChildrenCollapsible(false);
    m_consoleSplitter->setHandleWidth(4);
    
    connect(ui->menuHelp,&QMenu::aboutToShow, this, &MainWindowView::helpClicked);
    connect(ui->actionComs, &QAction::triggered, this, &MainWindowView::comsClicked);
    connect(ui->actionConsole, &QAction::triggered, this, &MainWindowView::consoleClicked);

    Logger::instance().log("CellCharger started");
}

MainWindowView::~MainWindowView()
{
    delete ui;
}

void MainWindowView::centerWindow(QWidget* child)
{
    if (!child) return;

    child->adjustSize();

    QPoint parentCenter = this->geometry().center();
    child->move(parentCenter.x() - child->width() / 2, parentCenter.y() - child->height() / 2);
}

void MainWindowView::showHelpWindow()
{
    HelpDialog dialog(this);
    dialog.exec();
    centerWindow(&dialog);

    Logger::instance().log("Help window opened");
}

void MainWindowView::showComsWindow()
{
    Coms *view = new Coms(this);
    ComsBackend *backend = new ComsBackend(this);

    new ComsController(view, backend, this);

    view->show();

    Logger::instance().log("Coms window opened");
}

void MainWindowView::closeConsolePanel(ConsoleWidget *panel)
{
    if (!panel) 
    {
        return;
    }

    panel->setParent(nullptr);
    panel->deleteLater();

    rebalanceConsolePanels();

    Logger::instance().log("Console closed");
}

void MainWindowView::rebalanceConsolePanels()
{
    const int count = m_consoleSplitter->count();
    if (count <= 0) 
    {
        return;
    }

    QList<int> sizes;
    for (int i = 0; i < count; ++i) 
    {
        sizes.append(width() / count);
    }

    m_consoleSplitter->setSizes(sizes);
}

void MainWindowView::openConsolePanel()
{
    auto *console = new ConsoleWidget(this);
    m_consoleSplitter->addWidget(console);

    connect(console, &ConsoleWidget::closeRequested, this, &MainWindowView::closeConsolePanel);

    rebalanceConsolePanels();

    Logger::instance().log("Console opened");
}