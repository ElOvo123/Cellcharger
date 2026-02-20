#pragma once

#include <QMainWindow>
#include <QVector>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QDockWidget;
class ConsoleWidget;

class MainWindowView : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindowView(QWidget *parent = nullptr);
    ~MainWindowView();

    void showHelpWindow();
    void showComsWindow();
    void openConsoleDock();

signals:
    void helpClicked();
    void comsClicked();
    void consoleClicked();

private:
    Ui::MainWindow *ui;

    QVector<QDockWidget*> m_consoleDocks;

    void centerWindow(QWidget* child);
    void ensureCentralCanCollapse();
};