#include <QApplication>
#include "mainwindow_controller.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MainWindowController main_window;
    main_window.show();

    return app.exec();
}
