#pragma once

#include <QMainWindow>
#include "panel_factory.h"
#include "pcp_database.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QSplitter;
class QLabel;
class QWidget;
class PanelContainer;
class Coms;
class ComsController;

class MainWindowView : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindowView(QWidget *parent = nullptr);
    ~MainWindowView();

    void showHelpWindow();
    void showComsWindow();

    void openPanel(PanelType type);

signals:
    void helpClicked();
    void comsClicked();
    void consoleClicked();
    void logClicked();
    void comsStatusClicked();

private:
    Ui::MainWindow *ui;
    QWidget *m_centralContainer = nullptr;
    QSplitter *m_panelSplitter;
    Coms *m_comsWindow = nullptr;
    ComsController *m_comsController = nullptr;
    PCPDatabase m_pcpDatabase;

    void centerWindow(QWidget* child);
    void addPanel(QWidget *contentWidget, const QString& title = QString());
    void removePanel(PanelContainer *panel);
    void rebalancePanels();
};
