#include "mainwindow_view.h"
#include "ui_mainwindow.h"
#include "help_dialog.h"
#include "logger_backend.h"
#include "coms.h"
#include "coms_controller.h"
#include "panel_container.h"
#include "panel_factory.h"
#include "simulated_coms_backend.h"
#include "console_widget.h"

#include <QList>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QMenu>
#include <QSplitter>

MainWindowView::MainWindowView(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow), m_panelSplitter(new QSplitter(Qt::Horizontal, this))
{
    ui->setupUi(this);

    ui->mainToolBar->setIconSize(QSize(48, 48));
    ui->mainToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    m_centralContainer = new QWidget(this);
    auto *centralLayout = new QVBoxLayout(m_centralContainer);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(m_panelSplitter);

    setCentralWidget(m_centralContainer);
    m_panelSplitter->setChildrenCollapsible(false);
    m_panelSplitter->setHandleWidth(4);
    m_panelSplitter->setMinimumWidth(0);
    m_panelSplitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(ui->menuHelp,&QMenu::aboutToShow, this, &MainWindowView::helpClicked);
    connect(ui->actionComs,&QAction::triggered, this, &MainWindowView::comsClicked);
    connect(ui->actionConsole, &QAction::triggered, this, &MainWindowView::consoleClicked);
    connect(ui->actionLog, &QAction::triggered, this, &MainWindowView::logClicked);
    connect(ui->actionComsStatus, &QAction::triggered, this, &MainWindowView::comsStatusClicked);

    Logger::instance().logStatus("CellCharger started");

    m_pcpDatabase.loadFromFile("pcp.yaml");
    ConsoleWidget::setSharedPCPDatabase(&m_pcpDatabase);

    Logger::instance().logStatus("PCP database loaded");
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

    Logger::instance().logStatus("Help window opened");
}

void MainWindowView::showComsWindow()
{
    if (!m_comsWindow)
    {
        m_comsWindow = new Coms(this);
        m_comsController = new ComsController(m_comsWindow, &m_pcpDatabase, this);
    }

    m_comsWindow->show();
    m_comsWindow->raise();
    m_comsWindow->activateWindow();

    Logger::instance().logStatus("Coms window opened");
}

void MainWindowView::openPanel(PanelType type)
{
    QWidget *contentWidget = PanelFactory::createPanelWidget(type, this);
    
    if (!contentWidget) 
    {
        Logger::instance().logStatus("Failed to create panel");
        return;
    }

    addPanel(contentWidget, PanelFactory::panelTitle(type));
    Logger::instance().logStatus(PanelFactory::panelTitle(type) + " opened");
}

void MainWindowView::addPanel(QWidget *contentWidget, const QString& title)
{
    if (!contentWidget) 
    {
        return;
    }

    auto *panel = new PanelContainer(title, this);
    panel->setContentWidget(contentWidget);
    panel->setMinimumWidth(0);

    m_panelSplitter->addWidget(panel);
    for (int i = 0; i < m_panelSplitter->count(); ++i)
        m_panelSplitter->setStretchFactor(i, 1);

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

    Logger::instance().logStatus("Panel closed");
}

void MainWindowView::rebalancePanels()
{
    const int count = m_panelSplitter->count();
    
    if (count <= 0) 
    {
        return;
    }

    QList<int> sizes;
    const int splitterWidth = qMax(1, m_panelSplitter->size().width());
    for (int i = 0; i < count; ++i) 
    {
        sizes.append(splitterWidth / count);
    }

    m_panelSplitter->setSizes(sizes);
}
