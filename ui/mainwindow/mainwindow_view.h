#pragma once

#include <QMainWindow>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QScreen>
#include <QWindow>
#include <QDialog>
#include <QLabel>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindowView : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindowView(QWidget *parent = nullptr);
    ~MainWindowView();
    void showHelpWindow();
    
signals:
    void helpClicked();
    void comsClicked();
    void consoleClicked();
    
private:
    Ui::MainWindow *ui;
    void centerWindow(QWidget* child);
};
