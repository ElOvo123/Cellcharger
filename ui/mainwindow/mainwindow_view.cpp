#include "mainwindow_view.h"
#include "ui_mainwindow.h"
#include "help_dialog.h"
#include "logger_backend.h"
#include "coms.h"
#include "coms_controller.h"
#include "panel_container.h"
#include "panel_factory.h"

#include <QList>
#include <QMenu>
#include <QSplitter>

MainWindowView::MainWindowView(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow), m_panelSplitter(new QSplitter(Qt::Horizontal, this))
{
    ui->setupUi(this);

    setCentralWidget(m_panelSplitter);
    m_panelSplitter->setChildrenCollapsible(false);
    m_panelSplitter->setHandleWidth(4);

    connect(ui->menuHelp,&QMenu::aboutToShow, this, &MainWindowView::helpClicked);
    connect(ui->actionComs,&QAction::triggered, this, &MainWindowView::comsClicked);
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

void MainWindowView::openConsolePanel()
{
    openPanel(PanelType::Console);
}

void MainWindowView::openPanel(PanelType type)
{
    QWidget *contentWidget = PanelFactory::createPanelWidget(type, this);
    
    if (!contentWidget) 
    {
        Logger::instance().log("Failed to create panel");
        return;
    }

    addPanel(contentWidget, PanelFactory::panelTitle(type));
    Logger::instance().log(PanelFactory::panelTitle(type) + " opened");
}

void MainWindowView::addPanel(QWidget *contentWidget, const QString& title)
{
    if (!contentWidget) 
    {
        return;
    }

    auto *panel = new PanelContainer(title, this);
    panel->setContentWidget(contentWidget);

    m_panelSplitter->addWidget(panel);

    connect(panel, &PanelContainer::closeRequested, this, &MainWindowView::removePanel);
    rebalancePanels();
}

void MainWindowView::removePanel(PanelContainer *panel)
{
    if (!panel) 
    {
        return;
    }

    panel->setParent(nullptr);
    panel->deleteLater();

    rebalancePanels();

    Logger::instance().log("Panel closed");
}

void MainWindowView::rebalancePanels()
{
    const int count = m_panelSplitter->count();
    
    if (count <= 0) 
    {
        return;
    }

    QList<int> sizes;
    for (int i = 0; i < count; ++i) 
    {
        sizes.append(width() / count);
    }

    m_panelSplitter->setSizes(sizes);
}