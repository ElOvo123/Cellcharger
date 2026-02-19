#include "coms.h"
#include "ui_coms.h"

#include <QComboBox>
#include <QPushButton>
#include <QSerialPortInfo>

Coms::Coms(QWidget *parent) : QDialog(parent), ui(new Ui::Coms)
{
    ui->setupUi(this);

    ui->comboBoxType->addItem("Serial", static_cast<int>(ComsType::Serial));
    ui->comboBoxType->addItem("Socket vcan", static_cast<int>(ComsType::Socket_vcan));
    ui->comboBoxType->addItem("Socket UDP", static_cast<int>(ComsType::Socket_UDP));
    ui->comboBoxType->addItem("Socket TCP", static_cast<int>(ComsType::Socket_TCP));

    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts())
    {
        ui->comboSerialPort->addItem(info.portName());
    }

    ui->editBaudrate->setText("115200");
    ui->connectButton->setCheckable(true);

    connect(ui->connectButton, &QPushButton::toggled, this, &Coms::connectToggled);
    connect(ui->comboBoxType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) { ui->stackedConfig->setCurrentIndex(index == 0 ? 0 : 1); emit typeChanged(index); });
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

void Coms::setStatusText(const QString &text)
{
    ui->labelStatus->setText(text);
}

void Coms::setConnectedUI(bool connected)
{
    ui->comboBoxType->setEnabled(!connected);
    ui->stackedConfig->setEnabled(!connected);

    if (connected)
        ui->connectButton->setText("Disconnect");
    else
        ui->connectButton->setText("Connect");
}

ComsConfig Coms::currentConfig() const
{
    ComsConfig config;

    config.serialPort = ui->comboSerialPort->currentText();
    config.baudrate = ui->editBaudrate->text().toInt();

    config.ip = ui->editIP->text();
    config.port = ui->editPort->text().toInt();

    return config;
}

