#include "profile_setup_widget.h"
#include "ui_profile_setup_widget.h"
#include "profile_plot_widget.h"
#include "profile_setup_logic.h"
#include "profile_yaml_logic.h"
#include "logger_backend.h"

#include <QComboBox>
#include <QFile>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QPainter>
#include <QPushButton>
#include <QStandardPaths>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <algorithm>

namespace
{
QIcon makeActiveStepIcon()
{
    QPixmap pixmap(14, 14);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#1d4ed8"));
    painter.drawEllipse(QRectF(2.0, 2.0, 10.0, 10.0));

    return QIcon(pixmap);
}
}

ProfileSetupWidget::ProfileSetupWidget(QWidget *parent, ProfileSetupDialogs* dialogs) :
    QWidget(parent),
    ui(new Ui::ProfileSetupWidget)
{
    ui->setupUi(this);
    m_activeStepIcon = makeActiveStepIcon();
    if (dialogs)
    {
        m_dialogs = dialogs;
    }
    else
    {
        m_ownedDialogs = std::make_unique<DefaultProfileSetupDialogs>();
        m_dialogs = m_ownedDialogs.get();
    }

    // Setup each slot
    setupSlot(1);
    setupSlot(2);
    setupSlot(3);

    connect(ui->saveProfileButton1, &QPushButton::clicked, this, &ProfileSetupWidget::saveProfile);
    connect(ui->loadProfileButton1, &QPushButton::clicked, this, &ProfileSetupWidget::loadProfile);
    connect(ui->saveProfileButton2, &QPushButton::clicked, this, &ProfileSetupWidget::saveProfile);
    connect(ui->loadProfileButton2, &QPushButton::clicked, this, &ProfileSetupWidget::loadProfile);
    connect(ui->saveProfileButton3, &QPushButton::clicked, this, &ProfileSetupWidget::saveProfile);
    connect(ui->loadProfileButton3, &QPushButton::clicked, this, &ProfileSetupWidget::loadProfile);

    // Connect control mode combo boxes to update plot when changed
    connect(ui->controlModeComboBox1, qOverload<int>(&QComboBox::currentIndexChanged), this, &ProfileSetupWidget::updatePlot1);
    connect(ui->controlModeComboBox2, qOverload<int>(&QComboBox::currentIndexChanged), this, &ProfileSetupWidget::updatePlot2);
    connect(ui->controlModeComboBox3, qOverload<int>(&QComboBox::currentIndexChanged), this, &ProfileSetupWidget::updatePlot3);
}

ProfileSetupWidget::~ProfileSetupWidget()
{
    delete ui;
}

void ProfileSetupWidget::setupSlot(int slotIndex)
{
    QTableWidget *table = nullptr;
    QWidget *plotWidget = nullptr;
    ProfilePlotWidget *&plotWidgetRef = (slotIndex == 1) ? m_plotWidget1 : (slotIndex == 2) ? m_plotWidget2 : m_plotWidget3;
    void (ProfileSetupWidget::*addSlot)() = (slotIndex == 1) ? &ProfileSetupWidget::addSetpoint1 : (slotIndex == 2) ? &ProfileSetupWidget::addSetpoint2 : &ProfileSetupWidget::addSetpoint3;
    void (ProfileSetupWidget::*removeSlot)() = (slotIndex == 1) ? &ProfileSetupWidget::removeSetpoint1 : (slotIndex == 2) ? &ProfileSetupWidget::removeSetpoint2 : &ProfileSetupWidget::removeSetpoint3;
    void (ProfileSetupWidget::*updateSlot)() = (slotIndex == 1) ? &ProfileSetupWidget::updatePlot1 : (slotIndex == 2) ? &ProfileSetupWidget::updatePlot2 : &ProfileSetupWidget::updatePlot3;
    void (ProfileSetupWidget::*startSlot)() = (slotIndex == 1) ? &ProfileSetupWidget::startTest1 : (slotIndex == 2) ? &ProfileSetupWidget::startTest2 : &ProfileSetupWidget::startTest3;
    void (ProfileSetupWidget::*pauseSlot)() = (slotIndex == 1) ? &ProfileSetupWidget::pauseTest1 : (slotIndex == 2) ? &ProfileSetupWidget::pauseTest2 : &ProfileSetupWidget::pauseTest3;
    void (ProfileSetupWidget::*resetSlot)() = (slotIndex == 1) ? &ProfileSetupWidget::resetTest1 : (slotIndex == 2) ? &ProfileSetupWidget::resetTest2 : &ProfileSetupWidget::resetTest3;
    QPushButton *addButton = (slotIndex == 1) ? ui->addSetpointButton1 : (slotIndex == 2) ? ui->addSetpointButton2 : ui->addSetpointButton3;
    QPushButton *removeButton = (slotIndex == 1) ? ui->removeSetpointButton1 : (slotIndex == 2) ? ui->removeSetpointButton2 : ui->removeSetpointButton3;
    QPushButton *updateButton = (slotIndex == 1) ? ui->updatePlotButton1 : (slotIndex == 2) ? ui->updatePlotButton2 : ui->updatePlotButton3;
    QPushButton *startButton = (slotIndex == 1) ? ui->startTestButton1 : (slotIndex == 2) ? ui->startTestButton2 : ui->startTestButton3;
    QPushButton *pauseButton = (slotIndex == 1) ? ui->pauseTestButton1 : (slotIndex == 2) ? ui->pauseTestButton2 : ui->pauseTestButton3;
    QPushButton *resetButton = (slotIndex == 1) ? ui->resetTestButton1 : (slotIndex == 2) ? ui->resetTestButton2 : ui->resetTestButton3;

    if (slotIndex == 1) {
        table = ui->setpointsTable1;
        plotWidget = ui->plotWidget1;
    } else if (slotIndex == 2) {
        table = ui->setpointsTable2;
        plotWidget = ui->plotWidget2;
    } else {
        table = ui->setpointsTable3;
        plotWidget = ui->plotWidget3;
    }

    table->setColumnCount(6);
    table->setColumnCount(7);
    table->setHorizontalHeaderLabels({"", "Time (s)", "Voltage (V)", "Current (A)", "Temperature (°C)", "Curve to Before", "Ramp Step"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setMinimumSectionSize(80);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    table->setColumnWidth(0, 28);
    table->verticalHeader()->setVisible(false);
    table->setRowCount(0);

    SlotRuntime& runtime = m_slotRuntimes[static_cast<size_t>(slotIndex - 1)];
    runtime.table = table;
    runtime.activeRow = -1;
    runtime.elapsedSeconds = 0;
    runtime.paused = false;
    runtime.timer = new QTimer(this);
    runtime.timer->setInterval(1000);
    connect(runtime.timer, &QTimer::timeout, this,
            [this, slotIndex]()
            {
                SlotRuntime& activeRuntime = m_slotRuntimes[static_cast<size_t>(slotIndex - 1)];
                activeRuntime.elapsedSeconds += 1;
                updateActiveStepIndicator(slotIndex);
            });

    // Setup plot
    plotWidgetRef = new ProfilePlotWidget(plotWidget);
    auto *plotLayout = new QVBoxLayout(plotWidget);
    plotLayout->setContentsMargins(0, 0, 0, 0);
    plotLayout->addWidget(plotWidgetRef);

    connect(addButton, &QPushButton::clicked, this, addSlot);
    connect(removeButton, &QPushButton::clicked, this, removeSlot);
    connect(updateButton, &QPushButton::clicked, this, updateSlot);
    connect(startButton, &QPushButton::clicked, this, startSlot);
    connect(pauseButton, &QPushButton::clicked, this, pauseSlot);
    connect(resetButton, &QPushButton::clicked, this, resetSlot);

    // Call updatePlot for initial display
    (this->*updateSlot)();
}

void ProfileSetupWidget::addSetpoint1() { addSetpointRow(1, ui->setpointsTable1->rowCount()); }
void ProfileSetupWidget::addSetpoint2() { addSetpointRow(2, ui->setpointsTable2->rowCount()); }
void ProfileSetupWidget::addSetpoint3() { addSetpointRow(3, ui->setpointsTable3->rowCount()); }

void ProfileSetupWidget::removeSetpoint1()
{
    int row = ui->setpointsTable1->currentRow();
    if (row >= 0 && ui->setpointsTable1->rowCount() > 0) {
        ui->setpointsTable1->removeRow(row);
        updatePlot1();
    }
}

void ProfileSetupWidget::removeSetpoint2()
{
    int row = ui->setpointsTable2->currentRow();
    if (row >= 0 && ui->setpointsTable2->rowCount() > 0) {
        ui->setpointsTable2->removeRow(row);
        updatePlot2();
    }
}

void ProfileSetupWidget::removeSetpoint3()
{
    int row = ui->setpointsTable3->currentRow();
    if (row >= 0 && ui->setpointsTable3->rowCount() > 0) {
        ui->setpointsTable3->removeRow(row);
        updatePlot3();
    }
}

void ProfileSetupWidget::addSetpointRow(int slotIndex, int rowIndex)
{
    QTableWidget *table = nullptr;
    void (ProfileSetupWidget::*updateSlot)() = nullptr;

    if (slotIndex == 1) {
        table = ui->setpointsTable1;
        updateSlot = &ProfileSetupWidget::updatePlot1;
    } else if (slotIndex == 2) {
        table = ui->setpointsTable2;
        updateSlot = &ProfileSetupWidget::updatePlot2;
    } else {
        table = ui->setpointsTable3;
        updateSlot = &ProfileSetupWidget::updatePlot3;
    }

    table->insertRow(rowIndex);

    auto *activeItem = new QTableWidgetItem();
    activeItem->setFlags(Qt::ItemIsEnabled);
    activeItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(rowIndex, 0, activeItem);

    double prevTime = 0;
    double prevVolt = 4.2;
    double prevCurr = 1.0;
    double prevTemp = 25;

    if (rowIndex > 0) {
        if (auto *prevTimeSpin = qobject_cast<QDoubleSpinBox*>(table->cellWidget(rowIndex - 1, 1)))
            prevTime = prevTimeSpin->value();
        if (auto *prevVoltSpin = qobject_cast<QDoubleSpinBox*>(table->cellWidget(rowIndex - 1, 2)))
            prevVolt = prevVoltSpin->value();
        if (auto *prevCurrSpin = qobject_cast<QDoubleSpinBox*>(table->cellWidget(rowIndex - 1, 3)))
            prevCurr = prevCurrSpin->value();
        if (auto *prevTempSpin = qobject_cast<QDoubleSpinBox*>(table->cellWidget(rowIndex - 1, 4)))
            prevTemp = prevTempSpin->value();
    }

    auto *timeSpin = new QDoubleSpinBox();
    timeSpin->setRange(0, 3600);
    timeSpin->setValue(prevTime + 60.0);
    table->setCellWidget(rowIndex, 1, timeSpin);

    auto *voltSpin = new QDoubleSpinBox();
    voltSpin->setRange(0, 100);
    voltSpin->setValue(prevVolt);
    table->setCellWidget(rowIndex, 2, voltSpin);

    auto *currSpin = new QDoubleSpinBox();
    currSpin->setRange(0, 50);
    currSpin->setValue(prevCurr);
    table->setCellWidget(rowIndex, 3, currSpin);

    auto *tempSpin = new QDoubleSpinBox();
    tempSpin->setRange(0, 100);
    tempSpin->setValue(prevTemp);
    table->setCellWidget(rowIndex, 4, tempSpin);

    auto *curveCombo = new QComboBox();
    curveCombo->addItem("Ramp");
    curveCombo->addItem("Linear");
    curveCombo->addItem("Exponential");
    curveCombo->addItem("Step");
    curveCombo->setCurrentText("Ramp");
    table->setCellWidget(rowIndex, 5, curveCombo);

    auto *rampStepSpin = new QDoubleSpinBox();
    rampStepSpin->setRange(0.1, 100.0);
    rampStepSpin->setDecimals(2);
    rampStepSpin->setSingleStep(0.1);
    rampStepSpin->setValue(1.0);
    table->setCellWidget(rowIndex, 6, rampStepSpin);

    connect(timeSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, updateSlot);
    connect(voltSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, updateSlot);
    connect(currSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, updateSlot);
    connect(tempSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, updateSlot);
    connect(curveCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, updateSlot);
    connect(rampStepSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, updateSlot);

    (this->*updateSlot)();
}

void ProfileSetupWidget::updatePlot1() { updatePlotForSlot(1, ui->setpointsTable1, m_plotWidget1); }
void ProfileSetupWidget::updatePlot2() { updatePlotForSlot(2, ui->setpointsTable2, m_plotWidget2); }
void ProfileSetupWidget::updatePlot3() { updatePlotForSlot(3, ui->setpointsTable3, m_plotWidget3); }

void ProfileSetupWidget::updatePlotForSlot(int slotIndex, QTableWidget *table, ProfilePlotWidget *plotWidget)
{
    std::vector<Setpoint> setpoints;
    for (int row = 0; row < table->rowCount(); ++row) {
        Setpoint sp;
        if (auto *timeSpin = qobject_cast<QDoubleSpinBox*>(table->cellWidget(row, 1)))
            sp.time = timeSpin->value();
        if (auto *voltSpin = qobject_cast<QDoubleSpinBox*>(table->cellWidget(row, 2)))
            sp.voltage = voltSpin->value();
        if (auto *currSpin = qobject_cast<QDoubleSpinBox*>(table->cellWidget(row, 3)))
            sp.current = currSpin->value();
        if (auto *tempSpin = qobject_cast<QDoubleSpinBox*>(table->cellWidget(row, 4)))
            sp.temperature = tempSpin->value();
        if (auto *curveCombo = qobject_cast<QComboBox*>(table->cellWidget(row, 5)))
            sp.curveType = curveCombo->currentText();
        if (auto *rampStepSpin = qobject_cast<QDoubleSpinBox*>(table->cellWidget(row, 6)))
            sp.rampStep = rampStepSpin->value();
        setpoints.push_back(sp);
    }

    int index = 0;
    if (slotIndex == 1) index = ui->controlModeComboBox1->currentIndex();
    else if (slotIndex == 2) index = ui->controlModeComboBox2->currentIndex();
    else index = ui->controlModeComboBox3->currentIndex();
    const ProfilePlotWidget::DisplayMode mode = ProfileSetupLogic::displayModeForControlIndex(index);
    plotWidget->setDisplayMode(mode);
    plotWidget->setSetpoints(setpoints);
}

void ProfileSetupWidget::saveProfile()
{
    const QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString filePath = m_dialogs ? m_dialogs->getSaveFilePath(this, defaultDir) : QString();

    if (filePath.isEmpty())
        return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        showError("Error", "Could not open file for writing: " + filePath);
        return;
    }
    file.write(serializeProfileToYaml().toUtf8());
    file.close();
    showInfo("Success", "Profile saved successfully!");
}

QString ProfileSetupWidget::serializeProfileToYaml() const
{
    return ProfileYamlLogic::serialize(profileDocumentFromUi());
}

ProfileDocument ProfileSetupWidget::profileDocumentFromUi() const
{
    ProfileDocument document;
    document.slotDocuments[0].setpoints = setpointsForTable(ui->setpointsTable1);
    document.slotDocuments[0].profileName = ui->profileNameEdit1->text();
    document.slotDocuments[0].displayModeIndex = ui->controlModeComboBox1->currentIndex();

    document.slotDocuments[1].setpoints = setpointsForTable(ui->setpointsTable2);
    document.slotDocuments[1].profileName = ui->profileNameEdit2->text();
    document.slotDocuments[1].displayModeIndex = ui->controlModeComboBox2->currentIndex();

    document.slotDocuments[2].setpoints = setpointsForTable(ui->setpointsTable3);
    document.slotDocuments[2].profileName = ui->profileNameEdit3->text();
    document.slotDocuments[2].displayModeIndex = ui->controlModeComboBox3->currentIndex();
    return document;
}

void ProfileSetupWidget::applySetpointsToTable(QTableWidget *table,
                                               const std::vector<Setpoint>& setpoints,
                                               void (ProfileSetupWidget::*updateSlot)())
{
    if (!table)
        return;

    table->setRowCount(0);
    for (const Setpoint& point : setpoints)
    {
        table->insertRow(table->rowCount());
        const int row = table->rowCount() - 1;

        auto *activeItem = new QTableWidgetItem();
        activeItem->setFlags(Qt::ItemIsEnabled);
        activeItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, 0, activeItem);

        auto *timeSpin = new QDoubleSpinBox();
        timeSpin->setRange(0, 3600);
        timeSpin->setValue(point.time);
        table->setCellWidget(row, 1, timeSpin);
        connect(timeSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, updateSlot);

        auto *voltSpin = new QDoubleSpinBox();
        voltSpin->setRange(0, 100);
        voltSpin->setValue(point.voltage);
        table->setCellWidget(row, 2, voltSpin);
        connect(voltSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, updateSlot);

        auto *currSpin = new QDoubleSpinBox();
        currSpin->setRange(0, 50);
        currSpin->setValue(point.current);
        table->setCellWidget(row, 3, currSpin);
        connect(currSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, updateSlot);

        auto *tempSpin = new QDoubleSpinBox();
        tempSpin->setRange(0, 100);
        tempSpin->setValue(point.temperature);
        table->setCellWidget(row, 4, tempSpin);
        connect(tempSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, updateSlot);

        auto *curveCombo = new QComboBox();
        curveCombo->addItems({"Ramp", "Linear", "Exponential", "Step"});
        curveCombo->setCurrentText(point.curveType.isEmpty() ? "Ramp" : point.curveType);
        table->setCellWidget(row, 5, curveCombo);
        connect(curveCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, updateSlot);

        auto *rampStepSpin = new QDoubleSpinBox();
        rampStepSpin->setRange(0.1, 100.0);
        rampStepSpin->setDecimals(2);
        rampStepSpin->setSingleStep(0.1);
        rampStepSpin->setValue(point.rampStep <= 0.0 ? 1.0 : point.rampStep);
        table->setCellWidget(row, 6, rampStepSpin);
        connect(rampStepSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, updateSlot);
    }
}

void ProfileSetupWidget::applyProfileDocument(const ProfileDocument& document)
{
    stopTestForSlot(1);
    stopTestForSlot(2);
    stopTestForSlot(3);

    applySetpointsToTable(ui->setpointsTable1, document.slotDocuments[0].setpoints, &ProfileSetupWidget::updatePlot1);
    applySetpointsToTable(ui->setpointsTable2, document.slotDocuments[1].setpoints, &ProfileSetupWidget::updatePlot2);
    applySetpointsToTable(ui->setpointsTable3, document.slotDocuments[2].setpoints, &ProfileSetupWidget::updatePlot3);

    ui->profileNameEdit1->setText(document.slotDocuments[0].profileName);
    ui->profileNameEdit2->setText(document.slotDocuments[1].profileName);
    ui->profileNameEdit3->setText(document.slotDocuments[2].profileName);

    ui->controlModeComboBox1->setCurrentIndex(document.slotDocuments[0].displayModeIndex);
    ui->controlModeComboBox2->setCurrentIndex(document.slotDocuments[1].displayModeIndex);
    ui->controlModeComboBox3->setCurrentIndex(document.slotDocuments[2].displayModeIndex);

    updatePlot1();
    updatePlot2();
    updatePlot3();
}

bool ProfileSetupWidget::deserializeProfileFromYaml(const QString& yamlText, QString *errorMessage)
{
    ProfileDocument document;
    if (!ProfileYamlLogic::deserialize(yamlText, document, errorMessage))
        return false;

    applyProfileDocument(document);
    return true;
}

void ProfileSetupWidget::loadProfile()
{
    const QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString filePath = m_dialogs ? m_dialogs->getOpenFilePath(this, defaultDir) : QString();

    if (filePath.isEmpty())
        return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        showError("Error", "Could not open file for reading: " + filePath);
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QString errorMessage;
    if (!deserializeProfileFromYaml(QString::fromUtf8(data), &errorMessage)) {
        showError("Error", "Invalid YAML format: " + errorMessage);
        return;
    }

    showInfo("Success", "Profile loaded successfully!");
}

void ProfileSetupWidget::startTest1() { startTestForSlot(1, ui->setpointsTable1); }
void ProfileSetupWidget::startTest2() { startTestForSlot(2, ui->setpointsTable2); }
void ProfileSetupWidget::startTest3() { startTestForSlot(3, ui->setpointsTable3); }
void ProfileSetupWidget::pauseTest1() { pauseTestForSlot(1); }
void ProfileSetupWidget::pauseTest2() { pauseTestForSlot(2); }
void ProfileSetupWidget::pauseTest3() { pauseTestForSlot(3); }
void ProfileSetupWidget::resetTest1() { resetTestForSlot(1); }
void ProfileSetupWidget::resetTest2() { resetTestForSlot(2); }
void ProfileSetupWidget::resetTest3() { resetTestForSlot(3); }

void ProfileSetupWidget::startTestForSlot(int slotIndex, QTableWidget *table)
{
    if (table->rowCount() == 0) {
        showWarning("Warning", "Please add at least one setpoint before starting a test.");
        return;
    }

    stopTestForSlot(slotIndex);

    SlotRuntime& runtime = m_slotRuntimes[static_cast<size_t>(slotIndex - 1)];
    runtime.elapsedSeconds = 0;
    runtime.activeRow = -1;
    runtime.paused = false;
    if (QPushButton *pauseButton = findChild<QPushButton*>(QString("pauseTestButton%1").arg(slotIndex)))
        pauseButton->setText("Pause");
    updateActiveStepIndicator(slotIndex);
    if (runtime.timer)
        runtime.timer->start();
}

void ProfileSetupWidget::pauseTestForSlot(int slotIndex)
{
    SlotRuntime& runtime = m_slotRuntimes[static_cast<size_t>(slotIndex - 1)];
    if (!runtime.timer || runtime.activeRow < 0)
        return;

    runtime.paused = !runtime.paused;
    if (runtime.paused)
        runtime.timer->stop();
    else
        runtime.timer->start();

    if (QPushButton *pauseButton = findChild<QPushButton*>(QString("pauseTestButton%1").arg(slotIndex)))
        pauseButton->setText(runtime.paused ? "Resume" : "Pause");
}

void ProfileSetupWidget::resetTestForSlot(int slotIndex)
{
    SlotRuntime& runtime = m_slotRuntimes[static_cast<size_t>(slotIndex - 1)];
    if (!runtime.table || runtime.table->rowCount() == 0) {
        showWarning("Warning", "Please add at least one setpoint before resetting a test.");
        return;
    }

    runtime.elapsedSeconds = 0;
    runtime.activeRow = -1;
    runtime.paused = false;
    if (QPushButton *pauseButton = findChild<QPushButton*>(QString("pauseTestButton%1").arg(slotIndex)))
        pauseButton->setText("Pause");

    updateActiveStepIndicator(slotIndex);
    if (runtime.timer)
        runtime.timer->stop();
}

void ProfileSetupWidget::stopTestForSlot(int slotIndex)
{
    SlotRuntime& runtime = m_slotRuntimes[static_cast<size_t>(slotIndex - 1)];
    if (runtime.timer)
        runtime.timer->stop();

    runtime.elapsedSeconds = 0;
    runtime.activeRow = -1;
    runtime.paused = false;

    if (QPushButton *pauseButton = findChild<QPushButton*>(QString("pauseTestButton%1").arg(slotIndex)))
        pauseButton->setText("Pause");

    if (!runtime.table)
        return;

    for (int row = 0; row < runtime.table->rowCount(); ++row) {
        if (QTableWidgetItem *item = runtime.table->item(row, 0))
            item->setIcon(QIcon());
    }

    if (ProfilePlotWidget *plotWidget = plotWidgetForSlot(slotIndex))
        plotWidget->setActiveStepMarker(false, 0.0, 0.0);
}

int ProfileSetupWidget::activeStepRowForElapsedSeconds(QTableWidget *table, int elapsedSeconds) const
{
    return ProfileSetupLogic::activeRowForElapsedSeconds(setpointsForTable(table), elapsedSeconds);
}

ProfilePlotWidget* ProfileSetupWidget::plotWidgetForSlot(int slotIndex) const
{
    switch (slotIndex)
    {
        case 1:
            return m_plotWidget1;
        case 2:
            return m_plotWidget2;
        case 3:
            return m_plotWidget3;
        default:
            return nullptr;
    }
}

int ProfileSetupWidget::commandModeForSlot(int slotIndex) const
{
    QComboBox *controlMode = nullptr;
    switch (slotIndex)
    {
        case 1:
            controlMode = ui->controlModeComboBox1;
            break;
        case 2:
            controlMode = ui->controlModeComboBox2;
            break;
        case 3:
            controlMode = ui->controlModeComboBox3;
            break;
        default:
            break;
    }

    if (!controlMode)
        return 0;

    return ProfileSetupLogic::commandModeForControlIndex(controlMode->currentIndex());
}

std::vector<Setpoint> ProfileSetupWidget::setpointsForTable(QTableWidget *table) const
{
    std::vector<Setpoint> setpoints;
    if (!table)
        return setpoints;

    setpoints.reserve(table->rowCount());
    for (int row = 0; row < table->rowCount(); ++row)
    {
        Setpoint sp;
        if (auto *timeSpin = qobject_cast<QDoubleSpinBox*>(table->cellWidget(row, 1)))
            sp.time = timeSpin->value();
        if (auto *voltSpin = qobject_cast<QDoubleSpinBox*>(table->cellWidget(row, 2)))
            sp.voltage = voltSpin->value();
        if (auto *currSpin = qobject_cast<QDoubleSpinBox*>(table->cellWidget(row, 3)))
            sp.current = currSpin->value();
        if (auto *tempSpin = qobject_cast<QDoubleSpinBox*>(table->cellWidget(row, 4)))
            sp.temperature = tempSpin->value();
        if (auto *curveCombo = qobject_cast<QComboBox*>(table->cellWidget(row, 5)))
            sp.curveType = curveCombo->currentText();
        if (auto *rampStepSpin = qobject_cast<QDoubleSpinBox*>(table->cellWidget(row, 6)))
            sp.rampStep = rampStepSpin->value();
        setpoints.push_back(sp);
    }

    return setpoints;
}

double ProfileSetupWidget::displayValueForMode(const Setpoint& setpoint, ProfilePlotWidget::DisplayMode mode)
{
    return ProfileSetupLogic::displayValueForMode(setpoint, mode);
}

double ProfileSetupWidget::interpolateProfileValue(const std::vector<Setpoint>& setpoints,
                                                   double timeSeconds,
                                                   ProfilePlotWidget::DisplayMode mode)
{
    return ProfileSetupLogic::interpolateProfileValue(setpoints, timeSeconds, mode);
}

void ProfileSetupWidget::updateActiveStepIndicator(int slotIndex)
{
    SlotRuntime& runtime = m_slotRuntimes[static_cast<size_t>(slotIndex - 1)];
    if (!runtime.table)
        return;

    const std::vector<Setpoint> rawSetpoints = setpointsForTable(runtime.table);
    const ProfileStepState stepState =
        ProfileSetupLogic::stepStateForElapsedSeconds(rawSetpoints, runtime.elapsedSeconds,
                                                      slotIndex == 1 ? ui->controlModeComboBox1->currentIndex() :
                                                      slotIndex == 2 ? ui->controlModeComboBox2->currentIndex() :
                                                                       ui->controlModeComboBox3->currentIndex());
    const int row = stepState.activeRow;
    for (int i = 0; i < runtime.table->rowCount(); ++i) {
        if (QTableWidgetItem *item = runtime.table->item(i, 0))
            item->setIcon(i == row ? m_activeStepIcon : QIcon());
    }

    if (row < 0) {
        stopTestForSlot(slotIndex);
        return;
    }

    runtime.table->selectRow(row);
    runtime.activeRow = row;

    const auto *timeSpin = qobject_cast<QDoubleSpinBox*>(runtime.table->cellWidget(row, 1));
    const auto *voltSpin = qobject_cast<QDoubleSpinBox*>(runtime.table->cellWidget(row, 2));
    const auto *currSpin = qobject_cast<QDoubleSpinBox*>(runtime.table->cellWidget(row, 3));
    const auto *tempSpin = qobject_cast<QDoubleSpinBox*>(runtime.table->cellWidget(row, 4));
    const auto *curveCombo = qobject_cast<QComboBox*>(runtime.table->cellWidget(row, 5));
    const int mode = stepState.commandMode;
    const double markerTime = stepState.markerTime;
    const double setpoint = stepState.setpoint;
    const std::vector<Setpoint> normalizedSetpoints = ProfileSetupLogic::normalizedSetpoints(rawSetpoints);
    const double markerVoltage =
        ProfileSetupLogic::interpolateProfileValue(normalizedSetpoints, markerTime, ProfilePlotWidget::DisplayMode::Voltage);
    const double markerCurrent =
        ProfileSetupLogic::interpolateProfileValue(normalizedSetpoints, markerTime, ProfilePlotWidget::DisplayMode::Current);
    const double markerTemperature =
        ProfileSetupLogic::interpolateProfileValue(normalizedSetpoints, markerTime, ProfilePlotWidget::DisplayMode::Temperature);

    if (ProfilePlotWidget *plotWidget = plotWidgetForSlot(slotIndex))
    {
        plotWidget->setActiveStepMarkerValues(
            true,
            markerTime,
            markerVoltage,
            markerCurrent,
            markerTemperature);
    }

    const QString message = QString(
        "PROFILE slot %1 step %2/%3 | t=%4s | V=%5 | I=%6 | T=%7 | curve=%8 | send mode=%9 setpoint=%10")
        .arg(slotIndex)
        .arg(row + 1)
        .arg(runtime.table->rowCount())
        .arg(QString::number(markerTime, 'f', 1))
        .arg(voltSpin ? QString::number(voltSpin->value(), 'f', 3) : QString("--"))
        .arg(currSpin ? QString::number(currSpin->value(), 'f', 3) : QString("--"))
        .arg(tempSpin ? QString::number(tempSpin->value(), 'f', 1) : QString("--"))
        .arg(curveCombo ? curveCombo->currentText() : QString("--"))
        .arg(mode == 1 ? QString("CV") : QString("CC"))
        .arg(QString::number(setpoint, 'f', 3));

    std::cout << message.toStdString() << std::endl;
    Logger::instance().logStatus(message);
    emit commandRequested(static_cast<uint32_t>(slotIndex), mode, true, setpoint);

    if (stepState.finished)
    {
        if (runtime.timer)
            runtime.timer->stop();
    }
}

void ProfileSetupWidget::showError(const QString& title, const QString& message)
{
    if (m_dialogs)
        m_dialogs->showError(this, title, message);
}

void ProfileSetupWidget::showInfo(const QString& title, const QString& message)
{
    if (m_dialogs)
        m_dialogs->showInfo(this, title, message);
}

void ProfileSetupWidget::showWarning(const QString& title, const QString& message)
{
    if (m_dialogs)
        m_dialogs->showWarning(this, title, message);
}
