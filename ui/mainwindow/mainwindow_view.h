#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QSplitter;
class ConsoleWidget;

class MainWindowView : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindowView(QWidget *parent = nullptr);
    ~MainWindowView();

    void showHelpWindow();
    void showComsWindow();
    void openConsolePanel();

signals:
    void helpClicked();
    void comsClicked();
    void consoleClicked();

private:
    Ui::MainWindow *ui;
    QSplitter *m_consoleSplitter;

    void centerWindow(QWidget* child);
    void closeConsolePanel(ConsoleWidget *panel);
    void rebalanceConsolePanels();
};