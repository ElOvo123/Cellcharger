#include "charger_status_widget.h"

#include "charger_history_plot_widget.h"
#include "charger_status_logic.h"
#include "coms_activity_widget.h"
#include "logger_backend.h"
#include "ui_charger_status_widget.h"

#include <QButtonGroup>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QStyle>
#include <QTabBar>
#include <QTabWidget>
#include <QTimer>

namespace
{
constexpr int kFreshTimeoutMs = 500;
}

ChargerStatusWidget::ChargerStatusWidget(QWidget* parent) : QWidget(parent), ui(new Ui::ChargerStatusWidget)
{
    ui->setupUi(this);

    setObjectName("comsStatusRoot");
    m_tabWidget = ui->statusTabWidget;
    setStyleSheet("QWidget#comsStatusRoot {"
                  "  background-color: palette(window);"
                  "}"
                  "QLabel#slotStatusLabel1, QLabel#slotStatusLabel2, QLabel#slotStatusLabel3 {"
                  "  color: palette(window-text);"
                  "  line-height: 1.35;"
                  "}"
                  "QFrame#generalColumn1, QFrame#generalColumn2, QFrame#generalColumn3 {"
                  "  background-color: palette(base);"
                  "  border: 1px solid #c8c8c8;"
                  "}"
                  "QLabel#generalTitleLabel1, QLabel#generalTitleLabel2, QLabel#generalTitleLabel3 {"
                  "  color: palette(window-text);"
                  "  font-weight: 600;"
                  "}"
                  "QLabel#generalVoltageLabel1, QLabel#generalVoltageLabel2, QLabel#generalVoltageLabel3,"
                  "QLabel#generalCurrentLabel1, QLabel#generalCurrentLabel2, QLabel#generalCurrentLabel3,"
                  "QLabel#generalTempLabel1, QLabel#generalTempLabel2, QLabel#generalTempLabel3,"
                  "QLabel#detailedTabPlaceholderLabel {"
                  "  color: palette(window-text);"
                  "}"
                  "QWidget#detailedTab {"
                  "  background-color: palette(window);"
                  "}"
                  "QFrame#detailedControlsFrame {"
                  "  background-color: palette(base);"
                  "  border: 1px solid #c8c8c8;"
                  "}"
                  "QTabWidget#detailedChargerTabWidget::pane {"
                  "  border: 1px solid #c8c8c8;"
                  "  background-color: palette(base);"
                  "}"
                  "QDoubleSpinBox#detailedSetpointSpinBox {"
                  "  min-height: 30px;"
                  "  background-color: white;"
                  "  border: 1px solid #b8b8b8;"
                  "  padding: 2px 8px;"
                  "}"
                  "QPushButton#detailedSetpointMinusButton, QPushButton#detailedSetpointPlusButton {"
                  "  min-height: 30px;"
                  "  min-width: 34px;"
                  "  background-color: white;"
                  "  border: 1px solid #b8b8b8;"
                  "  font-weight: 700;"
                  "}"
                  "QPushButton#detailedSetSetpointButton {"
                  "  min-height: 30px;"
                  "  background-color: white;"
                  "  border: 1px solid #b8b8b8;"
                  "  padding: 0 12px;"
                  "}"
                  "QPushButton#detailedModeCcButton, QPushButton#detailedModeCvButton {"
                  "  min-height: 30px;"
                  "  min-width: 58px;"
                  "  background-color: white;"
                  "  border: 1px solid #b8b8b8;"
                  "  font-weight: 600;"
                  "}"
                  "QPushButton#detailedModeCcButton:checked, QPushButton#detailedModeCvButton:checked {"
                  "  background-color: #e9e9e9;"
                  "  color: palette(button-text);"
                  "  border: 1px solid #9d9d9d;"
                  "}"
                  "QPushButton#detailedStartButton, QPushButton#detailedStopButton {"
                  "  min-height: 32px;"
                  "}");
    setupTabWidget();
    setupCommandButtons();
    setupDetailedControls();

    m_activityWidget = new ComsActivityWidget(ui->activityHost);
    m_activityWidget->setObjectName("activityIndicator");
    ui->activityHostLayout->addWidget(m_activityWidget, 0, Qt::AlignCenter);
    m_historyTimer.start();

    m_slots[0].led = ui->slotLed1;
    m_slots[0].overviewLabel = ui->slotStatusLabel1;
    m_slots[0].generalTitleLabel = ui->generalTitleLabel1;
    m_slots[0].generalVoltageLabel = ui->generalVoltageLabel1;
    m_slots[0].generalCurrentLabel = ui->generalCurrentLabel1;
    m_slots[0].generalTempLabel = ui->generalTempLabel1;
    m_slots[1].led = ui->slotLed2;
    m_slots[1].overviewLabel = ui->slotStatusLabel2;
    m_slots[1].generalTitleLabel = ui->generalTitleLabel2;
    m_slots[1].generalVoltageLabel = ui->generalVoltageLabel2;
    m_slots[1].generalCurrentLabel = ui->generalCurrentLabel2;
    m_slots[1].generalTempLabel = ui->generalTempLabel2;
    m_slots[2].led = ui->slotLed3;
    m_slots[2].overviewLabel = ui->slotStatusLabel3;
    m_slots[2].generalTitleLabel = ui->generalTitleLabel3;
    m_slots[2].generalVoltageLabel = ui->generalVoltageLabel3;
    m_slots[2].generalCurrentLabel = ui->generalCurrentLabel3;
    m_slots[2].generalTempLabel = ui->generalTempLabel3;

    for (int i = 0; i < static_cast<int>(m_slots.size()); ++i)
    {
        m_slots[static_cast<size_t>(i)].timer = new QTimer(this);
        m_slots[static_cast<size_t>(i)].timer->setSingleShot(true);
        m_slots[static_cast<size_t>(i)].led->setAttribute(Qt::WA_StyledBackground, true);
        m_slots[static_cast<size_t>(i)].led->setProperty("active", false);

        connect(m_slots[static_cast<size_t>(i)].timer, &QTimer::timeout, this,
                [this, i]() { applySlotState(i, false); });
    }

    clearSlots();

    connect(&Logger::instance(), &Logger::newStatusMessage, this,
            [this](const QString& message)
            {
                const QString lower = message.toLower();
                if (!lower.contains("coms:"))
                    return;

                if (lower.contains("disconnected") || lower.contains("connection failed") || lower.contains("no pcp") ||
                    lower.contains("no devices") || lower.contains("error"))
                {
                    clearSlots();
                }
            });

    connect(&Logger::instance(), &Logger::newDecodedMessage, this,
            [this](const QString& message) { processDecodedMessage(message); });
}

ChargerStatusWidget::~ChargerStatusWidget()
{
    delete ui;
}

void ChargerStatusWidget::setupTabWidget()
{
    if (!m_tabWidget)
        return;

    m_tabWidget->setCurrentIndex(0);
    m_tabWidget->tabBar()->setExpanding(false);
}

void ChargerStatusWidget::setupCommandButtons()
{
    connect(ui->startButton1, &QPushButton::clicked, this, [this]() { emitCommandForSlot(0, 0, true); });
    connect(ui->stopButton1, &QPushButton::clicked, this, [this]() { emitCommandForSlot(0, 0, false); });
    connect(ui->irButton1, &QPushButton::clicked, this, [this]() { emitCommandForSlot(0, 2, true); });
    connect(ui->ecmButton1, &QPushButton::clicked, this, [this]() { emitCommandForSlot(0, 3, true); });
    connect(ui->capacityButton1, &QPushButton::clicked, this, [this]() { emitCommandForSlot(0, 4, true); });
    connect(ui->enduranceTestButton1, &QPushButton::clicked, this, [this]() { emitCommandForSlot(0, 5, true); });

    connect(ui->startButton2, &QPushButton::clicked, this, [this]() { emitCommandForSlot(1, 0, true); });
    connect(ui->stopButton2, &QPushButton::clicked, this, [this]() { emitCommandForSlot(1, 0, false); });
    connect(ui->irButton2, &QPushButton::clicked, this, [this]() { emitCommandForSlot(1, 2, true); });
    connect(ui->ecmButton2, &QPushButton::clicked, this, [this]() { emitCommandForSlot(1, 3, true); });
    connect(ui->capacityButton2, &QPushButton::clicked, this, [this]() { emitCommandForSlot(1, 4, true); });
    connect(ui->enduranceTestButton2, &QPushButton::clicked, this, [this]() { emitCommandForSlot(1, 5, true); });

    connect(ui->startButton3, &QPushButton::clicked, this, [this]() { emitCommandForSlot(2, 0, true); });
    connect(ui->stopButton3, &QPushButton::clicked, this, [this]() { emitCommandForSlot(2, 0, false); });
    connect(ui->irButton3, &QPushButton::clicked, this, [this]() { emitCommandForSlot(2, 2, true); });
    connect(ui->ecmButton3, &QPushButton::clicked, this, [this]() { emitCommandForSlot(2, 3, true); });
    connect(ui->capacityButton3, &QPushButton::clicked, this, [this]() { emitCommandForSlot(2, 4, true); });
    connect(ui->enduranceTestButton3, &QPushButton::clicked, this, [this]() { emitCommandForSlot(2, 5, true); });
}

void ChargerStatusWidget::setupDetailedControls()
{
    m_historyPlots[0] = new ChargerHistoryPlotWidget(ui->detailedPlotHost1);
    m_historyPlots[0]->setObjectName("detailedHistoryPlot");
    m_historyPlots[0]->setSelectedCharger(1);
    ui->detailedPlotHostLayout1->addWidget(m_historyPlots[0]);

    m_historyPlots[1] = new ChargerHistoryPlotWidget(ui->detailedPlotHost2);
    m_historyPlots[1]->setObjectName("detailedHistoryPlot2");
    m_historyPlots[1]->setSelectedCharger(2);
    ui->detailedPlotHostLayout2->addWidget(m_historyPlots[1]);

    m_historyPlots[2] = new ChargerHistoryPlotWidget(ui->detailedPlotHost3);
    m_historyPlots[2]->setObjectName("detailedHistoryPlot3");
    m_historyPlots[2]->setSelectedCharger(3);
    ui->detailedPlotHostLayout3->addWidget(m_historyPlots[2]);

    ui->detailedChargerTabWidget->tabBar()->setExpanding(false);

    auto* modeGroup = new QButtonGroup(this);
    modeGroup->setExclusive(true);
    modeGroup->addButton(ui->detailedModeCcButton, 0);
    modeGroup->addButton(ui->detailedModeCvButton, 1);
    ui->detailedModeCcButton->setChecked(true);

    ui->detailedSetpointSpinBox->setDecimals(3);
    ui->detailedSetpointSpinBox->setRange(0.0, 1000000.0);
    ui->detailedSetpointSpinBox->setSingleStep(0.05);
    ui->detailedSetpointSpinBox->setValue(4.2);
    ui->detailedSetpointSpinBox->setMinimumWidth(96);
    ui->detailedSetpointSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    updateDetailedSetpointLabel();

    connect(ui->detailedModeCcButton, &QPushButton::toggled, this,
            [this](bool checked)
            {
                if (checked)
                    updateDetailedSetpointLabel();
            });
    connect(ui->detailedModeCvButton, &QPushButton::toggled, this,
            [this](bool checked)
            {
                if (checked)
                    updateDetailedSetpointLabel();
            });
    connect(ui->detailedSetpointMinusButton, &QPushButton::clicked, this,
            [this]()
            {
                if (ui->detailedSetpointSpinBox)
                    ui->detailedSetpointSpinBox->stepDown();
            });
    connect(ui->detailedSetpointPlusButton, &QPushButton::clicked, this,
            [this]()
            {
                if (ui->detailedSetpointSpinBox)
                    ui->detailedSetpointSpinBox->stepUp();
            });
    connect(ui->detailedSetSetpointButton, &QPushButton::clicked, this, [this]() { emitDetailedCommand(true); });
    connect(ui->detailedStartButton, &QPushButton::clicked, this, [this]() { emitDetailedCommand(true); });
    connect(ui->detailedStopButton, &QPushButton::clicked, this, [this]() { emitDetailedCommand(false); });
}

uint32_t ChargerStatusWidget::selectedDetailedChargerId() const
{
    if (!ui->detailedChargerTabWidget)
        return 1;

    const int index = ui->detailedChargerTabWidget->currentIndex();
    if (index < 0 || index >= static_cast<int>(m_historyPlots.size()))
        return 1;

    return static_cast<uint32_t>(index + 1);
}

void ChargerStatusWidget::updateDetailedSetpointLabel()
{
    if (!ui->detailedSetpointLabel || !ui->detailedModeCcButton || !ui->detailedModeCvButton)
        return;

    const bool cvMode = ui->detailedModeCvButton->isChecked();
    ui->detailedSetpointLabel->setText("Setpoint");
    if (ui->detailedSetpointSpinBox)
        ui->detailedSetpointSpinBox->setSuffix(QString());
    if (ui->detailedSetpointUnitLabel)
        ui->detailedSetpointUnitLabel->setText(cvMode ? "V" : "A");
}

void ChargerStatusWidget::clearSlots()
{
    for (int i = 0; i < static_cast<int>(m_slots.size()); ++i)
    {
        SlotWidgets& slot = m_slots[static_cast<size_t>(i)];
        slot.assigned = false;
        slot.deviceId = 0;
        if (slot.overviewLabel)
        {
            slot.overviewLabel->setText(ChargerStatusLogic::overviewMarkup("--", "--", "--", "--"));
        }
        if (slot.generalTitleLabel)
            slot.generalTitleLabel->setText(QString("Charger %1").arg(i + 1));
        if (slot.generalVoltageLabel)
            slot.generalVoltageLabel->setText("Voltage: --");
        if (slot.generalCurrentLabel)
            slot.generalCurrentLabel->setText("Current: --");
        if (slot.generalTempLabel)
            slot.generalTempLabel->setText("Temp: --");
        if (slot.timer)
            slot.timer->stop();
        applySlotState(i, false);
    }

    for (ChargerHistoryPlotWidget* plot : m_historyPlots)
    {
        if (plot)
            plot->clearHistory();
    }
    m_historyTimer.restart();
}

void ChargerStatusWidget::applySlotState(int slotIndex, bool active)
{
    if (slotIndex < 0 || slotIndex >= static_cast<int>(m_slots.size()))
        return;

    QWidget* led = m_slots[static_cast<size_t>(slotIndex)].led;
    if (led)
    {
        led->setProperty("active", active);
        led->setStyleSheet(active ? "background-color: #35b56a; border: 1px solid #1f7a45; border-radius: 13px;"
                                  : "background-color: #d95c5c; border: 1px solid #7b1f1f; border-radius: 13px;");
        led->style()->unpolish(led);
        led->style()->polish(led);
        led->update();
    }

    m_activityWidget->setSlotFresh(slotIndex, active);
}

int ChargerStatusWidget::slotIndexForDevice(uint32_t deviceId)
{
    for (int i = 0; i < static_cast<int>(m_slots.size()); ++i)
    {
        const SlotWidgets& slot = m_slots[static_cast<size_t>(i)];
        if (slot.assigned && slot.deviceId == deviceId)
            return i;
    }

    for (int i = 0; i < static_cast<int>(m_slots.size()); ++i)
    {
        SlotWidgets& slot = m_slots[static_cast<size_t>(i)];
        if (slot.assigned)
            continue;

        slot.assigned = true;
        slot.deviceId = deviceId;
        return i;
    }

    return -1;
}

void ChargerStatusWidget::updateSlotLabel(int slotIndex, const QString& voltText, const QString& currentText,
                                          const QString& tempText, const QString& statusText)
{
    if (slotIndex < 0 || slotIndex >= static_cast<int>(m_slots.size()))
        return;

    SlotWidgets& slot = m_slots[static_cast<size_t>(slotIndex)];
    if (slot.overviewLabel)
        slot.overviewLabel->setText(ChargerStatusLogic::overviewMarkup(voltText, currentText, tempText, statusText));

    if (slot.generalTitleLabel)
    {
        const QString titleText = QString("Charger %1").arg(slotIndex + 1);
        slot.generalTitleLabel->setText(titleText);
    }

    if (slot.generalVoltageLabel)
        slot.generalVoltageLabel->setText(QString("Voltage: %1 V").arg(voltText));
    if (slot.generalCurrentLabel)
        slot.generalCurrentLabel->setText(QString("Current: %1 A").arg(currentText));
    if (slot.generalTempLabel)
        slot.generalTempLabel->setText(QString("Temp: %1 C").arg(tempText));
}

void ChargerStatusWidget::markSlotFresh(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= static_cast<int>(m_slots.size()))
        return;

    applySlotState(slotIndex, true);

    QTimer* timer = m_slots[static_cast<size_t>(slotIndex)].timer;
    if (timer)
        timer->start(kFreshTimeoutMs);
}

void ChargerStatusWidget::emitCommandForSlot(int slotIndex, int mode, bool start)
{
    if (slotIndex < 0 || slotIndex >= static_cast<int>(m_slots.size()))
        return;

    const SlotWidgets& slot = m_slots[static_cast<size_t>(slotIndex)];
    const ChargerStatusCommand command =
        ChargerStatusLogic::generalCommandForSlot(slotIndex, slot.assigned, slot.deviceId, mode, start);
    emit commandRequested(command.chargerId, command.mode, command.start, command.setpoint);
}

void ChargerStatusWidget::emitDetailedCommand(bool start)
{
    if (!ui->detailedSetpointSpinBox || !ui->detailedModeCcButton || !ui->detailedModeCvButton)
        return;

    const ChargerStatusCommand command =
        ChargerStatusLogic::detailedCommand(selectedDetailedChargerId(), ui->detailedModeCvButton->isChecked(),
                                            ui->detailedSetpointSpinBox->value(), start);
    emit commandRequested(command.chargerId, command.mode, command.start, command.setpoint);
}

void ChargerStatusWidget::appendHistorySample(uint32_t chargerId, const QMap<QString, QString>& signalValues)
{
    double voltage = 0.0;
    double current = 0.0;
    if (!ChargerStatusLogic::parseNumericSignal(signalValues, "voltage", voltage) &&
        !ChargerStatusLogic::parseNumericSignal(signalValues, "volt", voltage))
    {
        return;
    }

    if (!ChargerStatusLogic::parseNumericSignal(signalValues, "current", current))
        return;

    const double elapsedSeconds = static_cast<double>(m_historyTimer.elapsed()) / 1000.0;
    for (ChargerHistoryPlotWidget* plot : m_historyPlots)
    {
        if (plot)
            plot->appendSample(chargerId, elapsedSeconds, voltage, current);
    }
}

void ChargerStatusWidget::processDecodedMessage(const QString& message)
{
    const ParsedChargerStatusMessage parsed = ChargerStatusLogic::parseDecodedStatusMessage(message);
    if (!parsed.valid)
        return;

    const int slotIndex = slotIndexForDevice(parsed.chargerId);
    if (slotIndex < 0)
        return;

    appendHistorySample(parsed.chargerId, parsed.signalValues);
    updateSlotLabel(slotIndex, parsed.voltageText, parsed.currentText, parsed.tempText, parsed.statusText);
    markSlotFresh(slotIndex);
}
