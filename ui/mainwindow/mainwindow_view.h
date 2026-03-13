#pragma once

#include <QMainWindow>
#include "panel_factory.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QSplitter;
class QWidget;
class PanelContainer;

class MainWindowView : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindowView(QWidget *parent = nullptr);
    ~MainWindowView();

    void showHelpWindow();
    void showComsWindow();

    void openConsolePanel();
    void openLogPanel();
    void openPanel(PanelType type);

signals:
    void helpClicked();
    void comsClicked();
    void consoleClicked();
    void logClicked();

private:
    Ui::MainWindow *ui;
    QSplitter *m_panelSplitter;

    void centerWindow(QWidget* child);
    void addPanel(QWidget *contentWidget, const QString& title = QString());
    void removePanel(PanelContainer *panel);
    void rebalancePanels();
};