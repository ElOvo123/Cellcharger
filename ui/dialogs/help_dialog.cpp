#include "help_dialog.h"
#include "ui_help_dialog.h"

HelpDialog::HelpDialog(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::HelpDialog)
{
    ui->setupUi(this);

    setWindowTitle("CellCharger Help");
    setFixedSize(520, 720);

    ui->valueVersion->setText("1.0");
    ui->valueBuild->setText(__DATE__ " " __TIME__);
}

HelpDialog::~HelpDialog()
{
    delete ui;
}
