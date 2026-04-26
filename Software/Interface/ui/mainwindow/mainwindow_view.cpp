#include "mainwindow_view.h"
#include "ui_mainwindow.h"
#include "charger_status_widget.h"
#include "help_dialog.h"
#include "logger_backend.h"
#include "coms.h"
#include "coms_controller.h"
#include "panel_container.h"
#include "panel_factory.h"
#include "simulated_coms_backend.h"
#include "console_widget.h"
#include "profile_setup_widget.h"

#include <QList>
#include <QSizePolicy>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QMenu>
#include <QSplitter>

namespace
{
const char kApplicationStyleSheet[] =
    "QMainWindow {"
    "  background-color: #f4f7fa;"
    "  color: #17212b;"
    "}"
    "QWidget {"
    "  font-family: \"Inter\", \"Segoe UI\", \"Roboto\", \"Helvetica Neue\", Arial, sans-serif;"
    "  font-size: 10pt;"
    "}"
    "QMenuBar {"
    "  background-color: #ffffff;"
    "  color: #17212b;"
    "  border-bottom: 1px solid #d6dee6;"
    "  padding: 3px 8px;"
    "}"
    "QMenuBar::item {"
    "  padding: 5px 10px;"
    "  background: transparent;"
    "}"
    "QMenuBar::item:selected {"
    "  background-color: #e8f1fb;"
    "}"
    "QMenu {"
    "  background-color: #ffffff;"
    "  color: #17212b;"
    "  border: 1px solid #cbd5df;"
    "}"
    "QMenu::item:selected {"
    "  background-color: #2a78c4;"
    "}"
    "QToolBar {"
    "  background-color: #ffffff;"
    "  border: 0;"
    "  border-bottom: 1px solid #d6dee6;"
    "  spacing: 6px;"
    "  padding: 7px 10px;"
    "}"
    "QToolButton {"
    "  color: #253341;"
    "  background-color: transparent;"
    "  border: 1px solid transparent;"
    "  border-radius: 2px;"
    "  padding: 5px 10px;"
    "  min-width: 76px;"
    "}"
    "QToolButton:hover {"
    "  background-color: #edf4fb;"
    "  border-color: #b9c8d6;"
    "}"
    "QToolButton:pressed {"
    "  background-color: #dceaf6;"
    "}"
    "QSplitter {"
    "  background-color: #f4f7fa;"
    "}"
    "QSplitter::handle {"
    "  background-color: #d6dee6;"
    "}"
    "QSplitter::handle:hover {"
    "  background-color: #3b83c7;"
    "}"
    "QStatusBar {"
    "  background-color: #ffffff;"
    "  color: #566575;"
    "  border-top: 1px solid #d6dee6;"
    "}"
    "QTabWidget::pane {"
    "  border: 1px solid #d6dee6;"
    "  background-color: #ffffff;"
    "}"
    "QTabBar::tab {"
    "  color: #566575;"
    "  background-color: #edf2f6;"
    "  border: 1px solid #d6dee6;"
    "  border-bottom: 0;"
    "  padding: 7px 14px;"
    "  margin-right: 3px;"
    "  min-width: 82px;"
    "}"
    "QTabBar::tab:selected {"
    "  color: #17212b;"
    "  background-color: #ffffff;"
    "  border-top: 2px solid #4aa3ff;"
    "}"
    "QPushButton {"
    "  min-height: 30px;"
    "  color: #17212b;"
    "  background-color: #eef3f7;"
    "  border: 1px solid #c4d0dc;"
    "  border-radius: 2px;"
    "  padding: 4px 12px;"
    "  font-weight: 600;"
    "}"
    "QPushButton:hover {"
    "  background-color: #e1edf7;"
    "  border-color: #9fb3c5;"
    "}"
    "QPushButton:pressed {"
    "  background-color: #d2e2f0;"
    "}"
    "QPushButton:disabled {"
    "  color: #9aa8b5;"
    "  background-color: #edf1f5;"
    "  border-color: #d8e0e8;"
    "}"
    "QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {"
    "  min-height: 28px;"
    "  color: #17212b;"
    "  background-color: #ffffff;"
    "  border: 1px solid #c4d0dc;"
    "  border-radius: 2px;"
    "  padding: 3px 8px;"
    "}"
    "QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {"
    "  border-color: #4aa3ff;"
    "}"
    "QHeaderView::section {"
    "  background-color: #e9eff5;"
    "  color: #17212b;"
    "  border: 0;"
    "  border-right: 1px solid #d6dee6;"
    "  padding: 6px 8px;"
    "  font-weight: 600;"
    "}"
    "QTableView, QTableWidget, QTextEdit {"
    "  color: #17212b;"
    "  background-color: #ffffff;"
    "  alternate-background-color: #f5f8fb;"
    "  border: 1px solid #d6dee6;"
    "  selection-background-color: #285f94;"
    "  selection-color: #ffffff;"
    "}"
    "QScrollBar:vertical, QScrollBar:horizontal {"
    "  background: #f4f7fa;"
    "  border: 0;"
    "}"
    "QScrollBar::handle {"
    "  background: #b9c8d6;"
    "  border-radius: 1px;"
    "}";
} // namespace

MainWindowView::MainWindowView(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_panelSplitter(new QSplitter(Qt::Horizontal, this))
{
    ui->setupUi(this);

    setWindowTitle("CellCharger Test Bench");
    resize(1280, 760);
    setStyleSheet(kApplicationStyleSheet);

    ui->mainToolBar->setIconSize(QSize(32, 32));
    ui->mainToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui->mainToolBar->setMovable(false);
    ui->mainToolBar->setFloatable(false);
    ui->statusbar->showMessage("Ready - industrial cell charger interface");

    m_centralContainer = new QWidget(this);
    m_centralContainer->setObjectName("centralWorkspace");
    auto* centralLayout = new QVBoxLayout(m_centralContainer);
    centralLayout->setContentsMargins(10, 10, 10, 10);
    centralLayout->setSpacing(10);
    centralLayout->addWidget(m_panelSplitter);

    setCentralWidget(m_centralContainer);
    m_panelSplitter->setChildrenCollapsible(false);
    m_panelSplitter->setHandleWidth(4);
    m_panelSplitter->setMinimumWidth(0);
    m_panelSplitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(ui->menuHelp, &QMenu::aboutToShow, this, &MainWindowView::helpClicked);
    connect(ui->actionComs, &QAction::triggered, this, &MainWindowView::comsClicked);
    connect(ui->actionConsole, &QAction::triggered, this, &MainWindowView::consoleClicked);
    connect(ui->actionLog, &QAction::triggered, this, &MainWindowView::logClicked);
    connect(ui->actionComsStatus, &QAction::triggered, this, &MainWindowView::comsStatusClicked);
    connect(ui->actionProfileSetup, &QAction::triggered, this, &MainWindowView::profileSetupClicked);

    m_pcpDatabase.loadFromFile("pcp.yaml");
    ConsoleWidget::setSharedPCPDatabase(&m_pcpDatabase);

    ensureComsController();
    Logger::instance().logStatus("CellCharger started");
    Logger::instance().logStatus("PCP database loaded");
}

MainWindowView::~MainWindowView()
{
    delete ui;
}

void MainWindowView::centerWindow(QWidget* child)
{
    if (!child)
        return;

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
    ensureComsController();

    m_comsWindow->show();
    m_comsWindow->raise();
    m_comsWindow->activateWindow();

    Logger::instance().logStatus("Coms window opened");
}

void MainWindowView::openPanel(PanelType type)
{
    QWidget* contentWidget = PanelFactory::createPanelWidget(type, this);

    if (!contentWidget)
    {
        Logger::instance().logStatus("Failed to create panel");
        return;
    }

    addPanel(contentWidget, PanelFactory::panelTitle(type));
    Logger::instance().logStatus(PanelFactory::panelTitle(type) + " opened");
}

void MainWindowView::addPanel(QWidget* contentWidget, const QString& title)
{
    if (!contentWidget)
    {
        return;
    }

    auto* panel = new PanelContainer(title, this);
    panel->setContentWidget(contentWidget);
    panel->setMinimumWidth(0);

    if (auto* chargerStatusWidget = qobject_cast<ChargerStatusWidget*>(contentWidget))
    {
        connect(chargerStatusWidget, &ChargerStatusWidget::commandRequested, this,
                [this](uint32_t chargerId, int mode, bool start, double setpoint)
                { dispatchChargerCommand(chargerId, mode, start, setpoint); });
    }

    if (auto* profileSetupWidget = qobject_cast<ProfileSetupWidget*>(contentWidget))
    {
        connect(profileSetupWidget, &ProfileSetupWidget::commandRequested, this,
                [this](uint32_t chargerId, int mode, bool start, double setpoint)
                { dispatchChargerCommand(chargerId, mode, start, setpoint); });
    }

    m_panelSplitter->addWidget(panel);
    for (int i = 0; i < m_panelSplitter->count(); ++i)
        m_panelSplitter->setStretchFactor(i, 1);

    connect(panel, &PanelContainer::closeRequested, this, &MainWindowView::removePanel);
    rebalancePanels();
}

void MainWindowView::ensureComsController()
{
    if (m_comsWindow && m_comsController)
        return;

    if (!m_comsWindow)
        m_comsWindow = new Coms(this);

    if (!m_comsController)
        m_comsController = new ComsController(m_comsWindow, &m_pcpDatabase, this);
}

void MainWindowView::removePanel(PanelContainer* panel)
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

void MainWindowView::dispatchChargerCommand(uint32_t chargerId, int mode, bool start, double setpoint)
{
    ensureComsController();
    if (!m_comsController)
        return;

    m_comsController->sendChargerCommand(chargerId, mode, start, setpoint);
}
