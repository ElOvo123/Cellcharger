#include "charger_status_widget.h"

#include "coms_activity_widget.h"
#include "logger_backend.h"
#include "ui_charger_status_widget.h"

#include <QFrame>
#include <QLabel>
#include <QRegularExpression>
#include <QString>
#include <QStyle>
#include <QTabBar>
#include <QTabWidget>
#include <QTimer>

namespace
{
constexpr int kFreshTimeoutMs = 500;

QString signalDisplay(const QMap<QString, QString>& signalValues,
                      const QString& primaryName,
                      const QString& fallbackName = QString(),
                      const QString& secondFallbackName = QString())
{
    if (signalValues.contains(primaryName))
        return signalValues.value(primaryName);

    if (!fallbackName.isEmpty() && signalValues.contains(fallbackName))
        return signalValues.value(fallbackName);

    if (!secondFallbackName.isEmpty() && signalValues.contains(secondFallbackName))
        return signalValues.value(secondFallbackName);

    return "--";
}

QString overviewMarkup(const QString& voltText,
                       const QString& currentText,
                       const QString& tempText,
                       const QString& statusText)
{
    return QString(
               "<span style='color:#486581;'>Voltage</span><br><b>%1 V</b><br>"
               "<span style='color:#486581;'>Current</span><br><b>%2 A</b><br>"
               "<span style='color:#486581;'>Temp</span><br><b>%3 C</b><br>"
               "<span style='color:#486581;'>Status</span><br><b>%4</b>")
        .arg(voltText)
        .arg(currentText)
        .arg(tempText)
        .arg(statusText);
}

QString formattedStatusText(const QMap<QString, QString>& signalValues)
{
    QString statusValue;
    if (signalValues.contains("status"))
        statusValue = signalValues.value("status");
    else if (signalValues.contains("state"))
        statusValue = signalValues.value("state");
    else
        return "--";

    bool ok = false;
    const int statusCode = statusValue.toInt(&ok);
    if (!ok)
        return statusValue;

    switch (statusCode)
    {
        case 0:
            return "Idle";
        case 1:
            return "Charging";
        case 2:
        {
            bool faultOk = false;
            const int faultCode = signalValues.value("fault_code", "0").toInt(&faultOk);
            return faultOk ? QString("Fault %1").arg(faultCode) : QString("Fault");
        }
        default:
            return QString::number(statusCode);
    }
}
}

ChargerStatusWidget::ChargerStatusWidget(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::ChargerStatusWidget)
{
    ui->setupUi(this);

    setObjectName("comsStatusRoot");
    m_tabWidget = ui->statusTabWidget;
    setStyleSheet(
        "QWidget#comsStatusRoot {"
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
    );
    setupTabWidget();

    m_activityWidget = new ComsActivityWidget(ui->activityHost);
    m_activityWidget->setObjectName("activityIndicator");
    ui->activityHostLayout->addWidget(m_activityWidget, 0, Qt::AlignCenter);

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
                [this, i]()
                {
                    applySlotState(i, false);
                });
    }

    clearSlots();

    connect(&Logger::instance(), &Logger::newStatusMessage, this,
            [this](const QString& message)
            {
                const QString lower = message.toLower();
                if (!lower.contains("coms:"))
                    return;

                if (lower.contains("disconnected") ||
                    lower.contains("connection failed") ||
                    lower.contains("no pcp") ||
                    lower.contains("no devices") ||
                    lower.contains("error"))
                {
                    clearSlots();
                }
            });

    connect(&Logger::instance(), &Logger::newDecodedMessage, this,
            [this](const QString& message)
            {
                processDecodedMessage(message);
            });
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

void ChargerStatusWidget::clearSlots()
{
    for (int i = 0; i < static_cast<int>(m_slots.size()); ++i)
    {
        SlotWidgets& slot = m_slots[static_cast<size_t>(i)];
        slot.assigned = false;
        slot.deviceId = 0;
        if (slot.overviewLabel)
        {
            slot.overviewLabel->setText(overviewMarkup("--", "--", "--", "--"));
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
}

void ChargerStatusWidget::applySlotState(int slotIndex, bool active)
{
    if (slotIndex < 0 || slotIndex >= static_cast<int>(m_slots.size()))
        return;

    QWidget *led = m_slots[static_cast<size_t>(slotIndex)].led;
    if (led)
    {
        led->setProperty("active", active);
        led->setStyleSheet(
            active
                ? "background-color: #35b56a; border: 1px solid #1f7a45; border-radius: 13px;"
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

void ChargerStatusWidget::updateSlotLabel(int slotIndex,
                                          const QString& voltText,
                                          const QString& currentText,
                                          const QString& tempText,
                                          const QString& statusText)
{
    if (slotIndex < 0 || slotIndex >= static_cast<int>(m_slots.size()))
        return;

    SlotWidgets& slot = m_slots[static_cast<size_t>(slotIndex)];
    if (slot.overviewLabel)
        slot.overviewLabel->setText(overviewMarkup(voltText, currentText, tempText, statusText));

    if (slot.generalTitleLabel)
    {
        const QString titleText = slot.assigned
            ? QString("Charger %1 · DEV %2").arg(slotIndex + 1).arg(slot.deviceId)
            : QString("Charger %1").arg(slotIndex + 1);
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

    QTimer *timer = m_slots[static_cast<size_t>(slotIndex)].timer;
    if (timer)
        timer->start(kFreshTimeoutMs);
}

void ChargerStatusWidget::processDecodedMessage(const QString& message)
{
    const QStringList lines = message.split('\n', Qt::SkipEmptyParts);
    if (lines.isEmpty())
        return;

    static const QRegularExpression headerRegex(
        "PCP\\s+([A-Za-z_][A-Za-z0-9_]*)\\s+\\|\\s+DEV=(\\d+)");
    const QRegularExpressionMatch headerMatch = headerRegex.match(lines.first());
    if (!headerMatch.hasMatch())
        return;

    const QString messageName = headerMatch.captured(1).toLower();
    if (messageName != "status")
        return;

    bool ok = false;
    const uint32_t headerDeviceId = headerMatch.captured(2).toUInt(&ok);
    if (!ok)
        return;

    QMap<QString, QString> signalValues;
    static const QRegularExpression signalRegex("^\\s*([A-Za-z0-9_]+)\\s*=\\s*([^\\s]+)");
    for (const QString& line : lines)
    {
        const QRegularExpressionMatch match = signalRegex.match(line);
        if (!match.hasMatch())
            continue;

        signalValues.insert(match.captured(1).toLower(), match.captured(2));
    }

    uint32_t chargerId = headerDeviceId;
    if (messageName == "status" && signalValues.contains("charger_id"))
        chargerId = signalValues.value("charger_id").toUInt(&ok);
    else if (messageName == "cell_info" && signalValues.contains("slot_id"))
        chargerId = signalValues.value("slot_id").toUInt(&ok);
    else
        ok = true;

    if (!ok)
        return;

    const int slotIndex = slotIndexForDevice(chargerId);
    if (slotIndex < 0)
        return;

    updateSlotLabel(
        slotIndex,
        signalDisplay(signalValues, "volt", "voltage"),
        signalDisplay(signalValues, "current"),
        signalDisplay(signalValues, "temp", "temperature"),
        formattedStatusText(signalValues));
    markSlotFresh(slotIndex);
}
