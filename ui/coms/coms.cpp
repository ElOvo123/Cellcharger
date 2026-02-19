#include "coms.h"
#include "ui_coms.h"

#include <QComboBox>

Coms::Coms(QWidget *parent) : QDialog(parent), ui(new Ui::Coms)
{
    ui->setupUi(this);

    setWindowTitle("Coms");
    setFixedSize(400, 60);

    ui->comboBoxType->addItem("Serial",static_cast<int>(ComsType::Serial));
    ui->comboBoxType->addItem("Socket", static_cast<int>(ComsType::Socket));

    connect(ui->comboBoxType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Coms::typeChanged);
}

Coms::~Coms()
{
    delete ui;
}

ComsType Coms::currentType() const
{
    return static_cast<ComsType>(
        ui->comboBoxType->currentData().toInt()
    );
}
