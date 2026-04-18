#include "profile_setup_widget.h"
#include "ui_profile_setup_widget.h"
#include "profile_plot_widget.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>

ProfileSetupWidget::ProfileSetupWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ProfileSetupWidget)
{
    ui->setupUi(this);

    ui->setpointsTable->setColumnCount(6);
    ui->setpointsTable->setHorizontalHeaderLabels({"Time (s)", "Voltage (V)", "Current (A)", "Temperature (°C)", "Curve to Before", "Ramp Step"});
    ui->setpointsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->setpointsTable->horizontalHeader()->setStretchLastSection(true);
    ui->setpointsTable->horizontalHeader()->setMinimumSectionSize(80);
    ui->setpointsTable->verticalHeader()->setVisible(false);
    ui->setpointsTable->setRowCount(0);

    // Setup plot
    m_plotWidget = new ProfilePlotWidget(ui->plotWidget);
    auto *plotLayout = new QVBoxLayout(ui->plotWidget);
    plotLayout->setContentsMargins(0, 0, 0, 0);
    plotLayout->addWidget(m_plotWidget);

    connect(ui->addSetpointButton, &QPushButton::clicked, this, &ProfileSetupWidget::addSetpoint);
    connect(ui->removeSetpointButton, &QPushButton::clicked, this, &ProfileSetupWidget::removeSetpoint);
    connect(ui->updatePlotButton, &QPushButton::clicked, this, &ProfileSetupWidget::updatePlot);
    connect(ui->controlModeComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &ProfileSetupWidget::updatePlot);
    connect(ui->saveProfileButton, &QPushButton::clicked, this, &ProfileSetupWidget::saveProfile);
    connect(ui->loadProfileButton, &QPushButton::clicked, this, &ProfileSetupWidget::loadProfile);
    connect(ui->startTestButton, &QPushButton::clicked, this, &ProfileSetupWidget::startTest);

    updatePlot();
}

ProfileSetupWidget::~ProfileSetupWidget()
{
    delete ui;
}

void ProfileSetupWidget::addSetpoint()
{
    int row = ui->setpointsTable->rowCount();
    ui->setpointsTable->insertRow(row);

    double prevTime = 0;
    double prevVolt = 4.2;
    double prevCurr = 1.0;
    double prevTemp = 25;

    if (row > 0) {
        if (auto *prevTimeSpin = qobject_cast<QDoubleSpinBox*>(ui->setpointsTable->cellWidget(row - 1, 0)))
            prevTime = prevTimeSpin->value();
        if (auto *prevVoltSpin = qobject_cast<QDoubleSpinBox*>(ui->setpointsTable->cellWidget(row - 1, 1)))
            prevVolt = prevVoltSpin->value();
        if (auto *prevCurrSpin = qobject_cast<QDoubleSpinBox*>(ui->setpointsTable->cellWidget(row - 1, 2)))
            prevCurr = prevCurrSpin->value();
        if (auto *prevTempSpin = qobject_cast<QDoubleSpinBox*>(ui->setpointsTable->cellWidget(row - 1, 3)))
            prevTemp = prevTempSpin->value();
    }

    auto *timeSpin = new QDoubleSpinBox();
    timeSpin->setRange(0, 3600);
    timeSpin->setValue(prevTime + 60.0);
    ui->setpointsTable->setCellWidget(row, 0, timeSpin);

    auto *voltSpin = new QDoubleSpinBox();
    voltSpin->setRange(0, 100);
    voltSpin->setValue(prevVolt);
    ui->setpointsTable->setCellWidget(row, 1, voltSpin);

    auto *currSpin = new QDoubleSpinBox();
    currSpin->setRange(0, 50);
    currSpin->setValue(prevCurr);
    ui->setpointsTable->setCellWidget(row, 2, currSpin);

    auto *tempSpin = new QDoubleSpinBox();
    tempSpin->setRange(0, 100);
    tempSpin->setValue(prevTemp);
    ui->setpointsTable->setCellWidget(row, 3, tempSpin);

    auto *curveCombo = new QComboBox();
    curveCombo->addItem("Ramp");
    curveCombo->addItem("Linear");
    curveCombo->addItem("Exponential");
    curveCombo->setCurrentText("Ramp");
    ui->setpointsTable->setCellWidget(row, 4, curveCombo);
    connect(curveCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &ProfileSetupWidget::updatePlot);

    auto *rampStepSpin = new QDoubleSpinBox();
    rampStepSpin->setRange(0.1, 100.0);
    rampStepSpin->setDecimals(2);
    rampStepSpin->setSingleStep(0.1);
    rampStepSpin->setValue(1.0);
    ui->setpointsTable->setCellWidget(row, 5, rampStepSpin);
    connect(rampStepSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ProfileSetupWidget::updatePlot);

    connect(timeSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ProfileSetupWidget::updatePlot);
    connect(voltSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ProfileSetupWidget::updatePlot);
    connect(currSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ProfileSetupWidget::updatePlot);
    connect(tempSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ProfileSetupWidget::updatePlot);

    updatePlot();
}

void ProfileSetupWidget::removeSetpoint()
{
    int row = ui->setpointsTable->currentRow();
    if (row >= 0 && ui->setpointsTable->rowCount() > 0) {
        ui->setpointsTable->removeRow(row);
        if (ui->setpointsTable->rowCount() > 0 && row == ui->setpointsTable->rowCount()) {
            ui->setpointsTable->setCellWidget(row - 1, 4, nullptr);
        }
        updatePlot();
    }
}

void ProfileSetupWidget::updatePlot()
{
    std::vector<Setpoint> setpoints;
    for (int row = 0; row < ui->setpointsTable->rowCount(); ++row) {
        Setpoint sp;
        if (auto *timeSpin = qobject_cast<QDoubleSpinBox*>(ui->setpointsTable->cellWidget(row, 0)))
            sp.time = timeSpin->value();
        if (auto *voltSpin = qobject_cast<QDoubleSpinBox*>(ui->setpointsTable->cellWidget(row, 1)))
            sp.voltage = voltSpin->value();
        if (auto *currSpin = qobject_cast<QDoubleSpinBox*>(ui->setpointsTable->cellWidget(row, 2)))
            sp.current = currSpin->value();
        if (auto *tempSpin = qobject_cast<QDoubleSpinBox*>(ui->setpointsTable->cellWidget(row, 3)))
            sp.temperature = tempSpin->value();
        if (auto *curveCombo = qobject_cast<QComboBox*>(ui->setpointsTable->cellWidget(row, 4)))
            sp.curveType = curveCombo->currentText();
        if (auto *rampStepSpin = qobject_cast<QDoubleSpinBox*>(ui->setpointsTable->cellWidget(row, 5)))
            sp.rampStep = rampStepSpin->value();
        setpoints.push_back(sp);
    }

    ProfilePlotWidget::DisplayMode mode = ProfilePlotWidget::DisplayMode::All;
    const int index = ui->controlModeComboBox->currentIndex();
    switch (index) {
        case 1: mode = ProfilePlotWidget::DisplayMode::Voltage; break;
        case 2: mode = ProfilePlotWidget::DisplayMode::Current; break;
        case 3: mode = ProfilePlotWidget::DisplayMode::Temperature; break;
        default: mode = ProfilePlotWidget::DisplayMode::All; break;
    }

    m_plotWidget->setDisplayMode(mode);
    m_plotWidget->setSetpoints(setpoints);
}

void ProfileSetupWidget::saveProfile()
{
    const QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString filePath = QFileDialog::getSaveFileName(
        this,
        "Save Profile",
        defaultDir + "/profile.json",
        "JSON Files (*.json);;All Files (*)"
    );

    if (filePath.isEmpty())
        return;

    QJsonArray setpointsArray;
    for (int row = 0; row < ui->setpointsTable->rowCount(); ++row) {
        QJsonObject obj;

        if (auto *timeSpin = qobject_cast<QDoubleSpinBox*>(ui->setpointsTable->cellWidget(row, 0)))
            obj["time"] = timeSpin->value();
        if (auto *voltSpin = qobject_cast<QDoubleSpinBox*>(ui->setpointsTable->cellWidget(row, 1)))
            obj["voltage"] = voltSpin->value();
        if (auto *currSpin = qobject_cast<QDoubleSpinBox*>(ui->setpointsTable->cellWidget(row, 2)))
            obj["current"] = currSpin->value();
        if (auto *tempSpin = qobject_cast<QDoubleSpinBox*>(ui->setpointsTable->cellWidget(row, 3)))
            obj["temperature"] = tempSpin->value();
        if (auto *curveCombo = qobject_cast<QComboBox*>(ui->setpointsTable->cellWidget(row, 4)))
            obj["curveType"] = curveCombo->currentText();
        if (auto *rampStepSpin = qobject_cast<QDoubleSpinBox*>(ui->setpointsTable->cellWidget(row, 5)))
            obj["rampStep"] = rampStepSpin->value();

        setpointsArray.append(obj);
    }

    QJsonObject root;
    root["setpoints"] = setpointsArray;
    root["displayMode"] = ui->controlModeComboBox->currentIndex();

    QJsonDocument doc(root);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Error", "Could not open file for writing: " + filePath);
        return;
    }

    file.write(doc.toJson());
    file.close();

    QMessageBox::information(this, "Success", "Profile saved successfully!");
}

void ProfileSetupWidget::loadProfile()
{
    const QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        "Load Profile",
        defaultDir,
        "JSON Files (*.json);;All Files (*)"
    );

    if (filePath.isEmpty())
        return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Error", "Could not open file for reading: " + filePath);
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        QMessageBox::critical(this, "Error", "Invalid JSON format");
        return;
    }

    QJsonObject root = doc.object();
    if (!root.contains("setpoints") || !root["setpoints"].isArray()) {
        QMessageBox::critical(this, "Error", "Invalid profile format: missing or invalid setpoints");
        return;
    }

    // Clear existing setpoints
    ui->setpointsTable->setRowCount(0);

    // Load setpoints
    const QJsonArray setpointsArray = root["setpoints"].toArray();
    for (const QJsonValue &value : setpointsArray) {
        if (!value.isObject())
            continue;

        const QJsonObject obj = value.toObject();

        ui->setpointsTable->insertRow(ui->setpointsTable->rowCount());
        int row = ui->setpointsTable->rowCount() - 1;

        // Time
        auto *timeSpin = new QDoubleSpinBox();
        timeSpin->setRange(0, 3600);
        timeSpin->setValue(obj["time"].toDouble(0.0));
        ui->setpointsTable->setCellWidget(row, 0, timeSpin);
        connect(timeSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ProfileSetupWidget::updatePlot);

        // Voltage
        auto *voltSpin = new QDoubleSpinBox();
        voltSpin->setRange(0, 100);
        voltSpin->setValue(obj["voltage"].toDouble(0.0));
        ui->setpointsTable->setCellWidget(row, 1, voltSpin);
        connect(voltSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ProfileSetupWidget::updatePlot);

        // Current
        auto *currSpin = new QDoubleSpinBox();
        currSpin->setRange(0, 50);
        currSpin->setValue(obj["current"].toDouble(0.0));
        ui->setpointsTable->setCellWidget(row, 2, currSpin);
        connect(currSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ProfileSetupWidget::updatePlot);

        // Temperature
        auto *tempSpin = new QDoubleSpinBox();
        tempSpin->setRange(0, 100);
        tempSpin->setValue(obj["temperature"].toDouble(0.0));
        ui->setpointsTable->setCellWidget(row, 3, tempSpin);
        connect(tempSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ProfileSetupWidget::updatePlot);

        // Curve Type
        auto *curveCombo = new QComboBox();
        curveCombo->addItem("Ramp");
        curveCombo->addItem("Linear");
        curveCombo->addItem("Exponential");
        curveCombo->setCurrentText(obj["curveType"].toString("Ramp"));
        ui->setpointsTable->setCellWidget(row, 4, curveCombo);
        connect(curveCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &ProfileSetupWidget::updatePlot);

        // Ramp Step
        auto *rampStepSpin = new QDoubleSpinBox();
        rampStepSpin->setRange(0.1, 100.0);
        rampStepSpin->setDecimals(2);
        rampStepSpin->setSingleStep(0.1);
        rampStepSpin->setValue(obj["rampStep"].toDouble(1.0));
        ui->setpointsTable->setCellWidget(row, 5, rampStepSpin);
        connect(rampStepSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ProfileSetupWidget::updatePlot);
    }

    // Load display mode
    if (root.contains("displayMode")) {
        ui->controlModeComboBox->setCurrentIndex(root["displayMode"].toInt(0));
    }

    updatePlot();
    QMessageBox::information(this, "Success", "Profile loaded successfully!");
}

void ProfileSetupWidget::startTest()
{
    if (ui->setpointsTable->rowCount() == 0) {
        QMessageBox::warning(this, "Warning", "Please add at least one setpoint before starting a test.");
        return;
    }

    QMessageBox::information(this, "Test Started", "Profile test initiated with " + QString::number(ui->setpointsTable->rowCount()) + " setpoint(s). Check the charger status panel for results.");
}