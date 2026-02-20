#include "mainwindow_view.h"
#include "ui_mainwindow.h"
#include "help_dialog.h"
#include "coms.h"
#include "coms_controller.h"

#include "console.h"
#include "logger_backend.h"

#include <QDockWidget>
#include <QMenu>
#include <QWidget>
#include <QSizePolicy>

MainWindowView::MainWindowView(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setDockNestingEnabled(true);
    ensureCentralCanCollapse();

    connect(ui->menuHelp, &QMenu::aboutToShow, this, &MainWindowView::helpClicked);
    connect(ui->actionComs, &QAction::triggered, this, &MainWindowView::comsClicked);
    connect(ui->actionConsole, &QAction::triggered, this, &MainWindowView::consoleClicked);

    Logger::instance().log("CellCharger started");
}

MainWindowView::~MainWindowView() {
    delete ui;
}

void MainWindowView::ensureCentralCanCollapse()
{
    QWidget *cw = centralWidget();
    if (!cw) {
        cw = new QWidget(this);
        setCentralWidget(cw);
    }

    cw->setMinimumSize(0, 0);
    cw->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    if (cw->layout())
        cw->layout()->setContentsMargins(0,0,0,0);
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

void MainWindowView::openConsoleDock()
{
    auto *dock = new QDockWidget(this);

    const int idx = m_consoleDocks.size() + 1;
    dock->setWindowTitle(QString("Console %1").arg(idx));

    dock->setAllowedAreas(Qt::BottomDockWidgetArea);
    dock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    auto *console = new ConsoleWidget(dock);
    dock->setWidget(console);

    addDockWidget(Qt::BottomDockWidgetArea, dock);

    if (!m_consoleDocks.isEmpty()) {
        tabifyDockWidget(m_consoleDocks.last(), dock);
        dock->raise();
    }

    m_consoleDocks.append(dock);

    int consoleHeight = int(height() * 0.85);
    resizeDocks({dock}, {consoleHeight}, Qt::Vertical);

    Logger::instance().log(QString("Console %1 opened").arg(idx));
}