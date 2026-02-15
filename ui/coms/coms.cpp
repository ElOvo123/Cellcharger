#include "coms.h"
#include "ui_coms.h"

Coms::Coms(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::Coms)
{
    ui->setupUi(this);

    setWindowTitle("Coms");
    setFixedSize(400, 60);
}

Coms::~Coms()
{
    delete ui;
}
