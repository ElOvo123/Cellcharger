#include <QtTest>

#define protected public
#define private public
#include "coms_activity_widget.h"
#include "charger_history_plot_widget.h"
#include "charger_status_logic.h"
#include "charger_status_widget.h"
#include "console_widget.h"
#include "help_dialog.h"
#include "logger_backend.h"
#include "log_widget.h"
#include "panel_container.h"
#include "pcp_database.h"
#undef private
#undef protected

#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPaintEvent>
#include <QPixmap>
#include <QPushButton>
#include <QSignalSpy>
#include <QTabWidget>
#include <QTableView>
#include <QTableWidget>
#include <QTimer>
#include <QTextEdit>

namespace
{
QImage renderWidgetImage(QWidget& widget)
{
    widget.resize(widget.sizeHint());
    QPixmap rendered(widget.size());
    rendered.fill(Qt::transparent);
    widget.render(&rendered);
    return rendered.toImage();
}
}

class WidgetTests : public QObject
{
    Q_OBJECT

private slots:
    void comsActivityWidget_turnsGreenOnFreshActivityAndRedAfterTimeout();
    void comsActivityWidget_ignoresInvalidIndexesAndOnlyEmitsOnStateChanges();
    void comsActivityWidget_exposesExpectedSizeHintsAndPaints();
    void chargerHistoryPlotWidget_capsSamplesSwitchesSelectionAndPaints();
    void chargerStatusLogic_parsesMessagesAndBuildsCommands();
    void chargerStatusLogic_coversFallbackAndInvalidBranches();
    void chargerStatusWidget_hasOverviewGeneralAndDetailedTabs();
    void chargerStatusWidget_generalButtonsEmitCommandsForAllSlots();
    void chargerStatusWidget_detailedControlsEmitSelectedCommandAndCollectHistory();
    void chargerStatusWidget_updatesFromLoggerTraffic();
    void chargerStatusWidget_ignoresInvalidDecodedMessagesAndExtraDevices();
    void helpDialog_initializesExpectedWindowState();
    void helpDialog_populatesProjectMetadata();
    void consoleWidget_appendsPausesFiltersAndClearsMessages();
    void consoleWidget_watchesDecodedSignalsAndManagesFilters();
    void consoleWidget_sendsManualAndPeriodicTxMessages();
    void logWidget_appendsFiltersPausesAndClearsMessages();
    void logWidget_startsWithPausedButtonUnchecked();
    void logWidget_loadsExistingLoggerHistoryOnConstruction();
    void panelContainer_setsContentAndEmitsCloseSignal();
    void panelContainer_ignoresNullContentWidgets();
};

void WidgetTests::comsActivityWidget_turnsGreenOnFreshActivityAndRedAfterTimeout()
{
    ComsActivityWidget widget;

    QVERIFY(!widget.isSlotFresh(0));
    QVERIFY(!widget.isSlotFresh(1));
    QVERIFY(!widget.isSlotFresh(2));

    widget.setSlotFresh(0, true);
    QVERIFY(widget.isSlotFresh(0));
    QVERIFY(!widget.isSlotFresh(1));

    widget.setSlotFresh(1, true);
    QVERIFY(widget.isSlotFresh(1));

    widget.clearSlots();
    QVERIFY(!widget.isSlotFresh(0));
    QVERIFY(!widget.isSlotFresh(1));
    QVERIFY(!widget.isSlotFresh(2));

    widget.setSlotFresh(2, true);
    QVERIFY(widget.isSlotFresh(2));
}

void WidgetTests::comsActivityWidget_ignoresInvalidIndexesAndOnlyEmitsOnStateChanges()
{
    ComsActivityWidget widget;
    QSignalSpy stateSpy(&widget, &ComsActivityWidget::visualStateChanged);

    widget.setSlotFresh(-1, true);
    widget.setSlotFresh(3, true);
    QCOMPARE(stateSpy.count(), 0);

    widget.setSlotFresh(0, true);
    QCOMPARE(stateSpy.count(), 1);

    widget.setSlotFresh(0, true);
    QCOMPARE(stateSpy.count(), 1);

    widget.clearSlots();
    QCOMPARE(stateSpy.count(), 2);

    widget.clearSlots();
    QCOMPARE(stateSpy.count(), 2);

    QVERIFY(!widget.isSlotFresh(-1));
    QVERIFY(!widget.isSlotFresh(42));
}

void WidgetTests::comsActivityWidget_exposesExpectedSizeHintsAndPaints()
{
    class InspectableComsActivityWidget : public ComsActivityWidget
    {
    public:
        using ComsActivityWidget::minimumSizeHint;
        using ComsActivityWidget::sizeHint;
    };

    InspectableComsActivityWidget widget;
    QCOMPARE(widget.minimumSizeHint(), QSize(290, 290));
    QCOMPARE(widget.sizeHint(), QSize(290, 290));

    const QImage idleImage = renderWidgetImage(widget);
    QVERIFY(!idleImage.isNull());

    widget.setSlotFresh(0, true);
    const QImage slot0FreshImage = renderWidgetImage(widget);
    QVERIFY(slot0FreshImage != idleImage);

    widget.setSlotFresh(2, true);
    const QImage slot0And2FreshImage = renderWidgetImage(widget);
    QVERIFY(slot0And2FreshImage != slot0FreshImage);

    widget.clearSlots();
    const QImage clearedImage = renderWidgetImage(widget);
    QCOMPARE(clearedImage, idleImage);
}

void WidgetTests::chargerHistoryPlotWidget_capsSamplesSwitchesSelectionAndPaints()
{
    ChargerHistoryPlotWidget widget;
    widget.resize(640, 360);

    QCOMPARE(widget.selectedCharger(), 1u);
    QCOMPARE(widget.sampleCountForCharger(0), 0);
    QCOMPARE(widget.sampleCountForCharger(4), 0);
    QVERIFY(widget.samplesForCharger(0).empty());

    QPixmap emptyPixmap(widget.size());
    emptyPixmap.fill(Qt::transparent);
    widget.render(&emptyPixmap);
    QVERIFY(!emptyPixmap.isNull());

    for (int i = 0; i < 200; ++i)
        widget.appendSample(1, static_cast<double>(i), 4.0 + (i * 0.01), 1.0 + (i * 0.02));
    QCOMPARE(widget.sampleCountForCharger(1), 180);
    QCOMPARE(widget.samplesForCharger(1).front().timeSeconds, 20.0);
    QCOMPARE(widget.samplesForCharger(1).back().timeSeconds, 199.0);

    widget.appendSample(0, 1.0, 1.0, 1.0);
    widget.appendSample(4, 1.0, 1.0, 1.0);
    QCOMPARE(widget.sampleCountForCharger(1), 180);

    widget.setSelectedCharger(2);
    QCOMPARE(widget.selectedCharger(), 2u);
    widget.setSelectedCharger(2);
    QCOMPARE(widget.selectedCharger(), 2u);
    widget.setSelectedCharger(4);
    QCOMPARE(widget.selectedCharger(), 2u);
    widget.appendSample(2, 10.0, 4.2, 1.5);
    widget.appendSample(2, 10.0, 4.2, 1.5);
    QCOMPARE(widget.sampleCountForCharger(2), 2);

    QPixmap dataPixmap(widget.size());
    dataPixmap.fill(Qt::transparent);
    widget.render(&dataPixmap);
    QVERIFY(!dataPixmap.isNull());

    QCOMPARE(ChargerHistoryPlotWidget::paddedLowerBound(5.0, 5.0), 4.99);
    QCOMPARE(ChargerHistoryPlotWidget::paddedUpperBound(5.0, 5.0), 5.01);

    widget.clearHistory();
    QCOMPARE(widget.sampleCountForCharger(1), 0);
    QCOMPARE(widget.sampleCountForCharger(2), 0);
    QCOMPARE(widget.sampleCountForCharger(3), 0);
}

void WidgetTests::chargerStatusLogic_parsesMessagesAndBuildsCommands()
{
    QMap<QString, QString> values;
    values.insert("voltage", "4.20");
    values.insert("temp", "25");
    values.insert("status", "2");
    values.insert("fault_code", "7");

    double numeric = 0.0;
    QVERIFY(ChargerStatusLogic::parseNumericSignal(values, "voltage", numeric));
    QCOMPARE(numeric, 4.2);
    QVERIFY(!ChargerStatusLogic::parseNumericSignal(values, "missing", numeric));
    QCOMPARE(ChargerStatusLogic::signalDisplay(values, "voltage"), QString("4.20"));
    QCOMPARE(ChargerStatusLogic::signalDisplay(values, "current", "temp"), QString("25"));
    QCOMPARE(ChargerStatusLogic::formattedStatusText(values), QString("Fault 7"));
    QVERIFY(ChargerStatusLogic::overviewMarkup("4.2", "1.0", "25", "Idle").contains("Voltage"));

    const ParsedChargerStatusMessage invalid = ChargerStatusLogic::parseDecodedStatusMessage("garbage");
    QVERIFY(!invalid.valid);

    const ParsedChargerStatusMessage parsed =
        ChargerStatusLogic::parseDecodedStatusMessage(
            "PCP status | DEV=1 | NAME=Charger Bus | MSG=18\n"
            "  charger_id = 3 (raw=3)\n"
            "  voltage = 4.12 (raw=4120)\n"
            "  current = 1.50 (raw=1500)\n"
            "  temperature = 26 (raw=260)\n"
            "  status = 1 (raw=1)");
    QVERIFY(parsed.valid);
    QCOMPARE(parsed.chargerId, 3u);
    QCOMPARE(parsed.voltageText, QString("4.12"));
    QCOMPARE(parsed.currentText, QString("1.50"));
    QCOMPARE(parsed.tempText, QString("26"));
    QCOMPARE(parsed.statusText, QString("Charging"));

    const ChargerStatusCommand general =
        ChargerStatusLogic::generalCommandForSlot(1, false, 9u, 2, true);
    QCOMPARE(general.chargerId, 2u);
    QCOMPARE(general.mode, 2);
    QVERIFY(general.start);
    QCOMPARE(general.setpoint, 4.2);

    const ChargerStatusCommand assigned =
        ChargerStatusLogic::generalCommandForSlot(1, true, 9u, 0, false);
    QCOMPARE(assigned.chargerId, 9u);
    QVERIFY(!assigned.start);
    QCOMPARE(assigned.setpoint, 0.0);

    const ChargerStatusCommand detailed =
        ChargerStatusLogic::detailedCommand(3u, true, 4.175, true);
    QCOMPARE(detailed.chargerId, 3u);
    QCOMPARE(detailed.mode, 1);
    QVERIFY(detailed.start);
    QCOMPARE(detailed.setpoint, 4.175);
}

void WidgetTests::chargerStatusLogic_coversFallbackAndInvalidBranches()
{
    QMap<QString, QString> values;
    values.insert("state", "2");
    values.insert("fault_code", "abc");
    values.insert("temperature", "hot");
    values.insert("alt_voltage", "4.18");

    double numeric = 9.0;
    QVERIFY(!ChargerStatusLogic::parseNumericSignal(values, "temperature", numeric));
    QCOMPARE(numeric, 0.0);
    QCOMPARE(ChargerStatusLogic::signalDisplay(values, "voltage", "missing", "alt_voltage"), QString("4.18"));
    QCOMPARE(ChargerStatusLogic::signalDisplay(values, "missing"), QString("--"));
    QCOMPARE(ChargerStatusLogic::formattedStatusText(values), QString("Fault"));

    values.insert("state", "text");
    QCOMPARE(ChargerStatusLogic::formattedStatusText(values), QString("text"));

    const ParsedChargerStatusMessage wrongMessage =
        ChargerStatusLogic::parseDecodedStatusMessage("PCP metrics | DEV=1\n  voltage = 4.1");
    QVERIFY(!wrongMessage.valid);

    const ParsedChargerStatusMessage badDevice =
        ChargerStatusLogic::parseDecodedStatusMessage(
            "PCP status | DEV=1 | NAME=Bus\n"
            "  charger_id = nope\n"
            "  current = 1.0\n"
            "  voltage = 4.1");
    QVERIFY(!badDevice.valid);

    const ParsedChargerStatusMessage fallbackParsed =
        ChargerStatusLogic::parseDecodedStatusMessage(
            "PCP status | DEV=2 | NAME=Bus\n"
            "  volt = 4.05\n"
            "  current = 0.80\n"
            "  temp = 24\n"
            "  state = 0");
    QVERIFY(fallbackParsed.valid);
    QCOMPARE(fallbackParsed.chargerId, 2u);
    QCOMPARE(fallbackParsed.voltageText, QString("4.05"));
    QCOMPARE(fallbackParsed.tempText, QString("24"));
    QCOMPARE(fallbackParsed.statusText, QString("Idle"));
}

void WidgetTests::chargerStatusWidget_hasOverviewGeneralAndDetailedTabs()
{
    auto* widget = new ChargerStatusWidget;

    auto* tabWidget = widget->findChild<QTabWidget*>("statusTabWidget");
    QVERIFY(tabWidget != nullptr);
    QCOMPARE(tabWidget->count(), 3);
    QCOMPARE(tabWidget->tabText(0), QString("Overview"));
    QCOMPARE(tabWidget->tabText(1), QString("General"));
    QCOMPARE(tabWidget->tabText(2), QString("Detailed"));

    auto* detailedPlot = widget->findChild<ChargerHistoryPlotWidget*>("detailedHistoryPlot");
    auto* detailedChargerTabs = widget->findChild<QTabWidget*>("detailedChargerTabWidget");
    auto* detailedModeCcButton = widget->findChild<QPushButton*>("detailedModeCcButton");
    auto* detailedModeCvButton = widget->findChild<QPushButton*>("detailedModeCvButton");
    auto* detailedSetpointSpin = widget->findChild<QDoubleSpinBox*>("detailedSetpointSpinBox");
    auto* detailedStartButton = widget->findChild<QPushButton*>("detailedStartButton");
    auto* detailedStopButton = widget->findChild<QPushButton*>("detailedStopButton");
    QVERIFY(detailedPlot != nullptr);
    QVERIFY(detailedChargerTabs != nullptr);
    QVERIFY(detailedModeCcButton != nullptr);
    QVERIFY(detailedModeCvButton != nullptr);
    QVERIFY(detailedSetpointSpin != nullptr);
    QVERIFY(detailedStartButton != nullptr);
    QVERIFY(detailedStopButton != nullptr);
    QCOMPARE(detailedChargerTabs->count(), 3);
    QCOMPARE(detailedChargerTabs->tabText(0), QString("Charger 1"));
    QCOMPARE(detailedChargerTabs->tabText(1), QString("Charger 2"));
    QCOMPARE(detailedChargerTabs->tabText(2), QString("Charger 3"));
    QCOMPARE(detailedModeCcButton->text(), QString("CC"));
    QCOMPARE(detailedModeCvButton->text(), QString("CV"));
    QVERIFY(detailedModeCcButton->isChecked());

    for (int column = 1; column <= 3; ++column)
    {
        auto* voltageLabel = widget->findChild<QLabel*>(QString("generalVoltageLabel%1").arg(column));
        auto* currentLabel = widget->findChild<QLabel*>(QString("generalCurrentLabel%1").arg(column));
        auto* tempLabel = widget->findChild<QLabel*>(QString("generalTempLabel%1").arg(column));
        auto* startButton = widget->findChild<QPushButton*>(QString("startButton%1").arg(column));
        auto* stopButton = widget->findChild<QPushButton*>(QString("stopButton%1").arg(column));
        auto* irButton = widget->findChild<QPushButton*>(QString("irButton%1").arg(column));
        auto* ecmButton = widget->findChild<QPushButton*>(QString("ecmButton%1").arg(column));
        auto* capacityButton = widget->findChild<QPushButton*>(QString("capacityButton%1").arg(column));

        QVERIFY(voltageLabel != nullptr);
        QVERIFY(currentLabel != nullptr);
        QVERIFY(tempLabel != nullptr);
        QVERIFY(startButton != nullptr);
        QVERIFY(stopButton != nullptr);
        QVERIFY(irButton != nullptr);
        QVERIFY(ecmButton != nullptr);
        QVERIFY(capacityButton != nullptr);

        QCOMPARE(startButton->text(), QString("Start"));
        QCOMPARE(stopButton->text(), QString("Stop"));
        QCOMPARE(irButton->text(), QString("IR"));
        QCOMPARE(ecmButton->text(), QString("ECM"));
        QCOMPARE(capacityButton->text(), QString("Capacity"));
    }

    delete widget;
}

void WidgetTests::chargerStatusWidget_generalButtonsEmitCommandsForAllSlots()
{
    ChargerStatusWidget widget;
    QSignalSpy commandSpy(&widget, &ChargerStatusWidget::commandRequested);

    auto* startButton1 = widget.findChild<QPushButton*>("startButton1");
    auto* stopButton2 = widget.findChild<QPushButton*>("stopButton2");
    auto* irButton3 = widget.findChild<QPushButton*>("irButton3");
    QVERIFY(startButton1 != nullptr);
    QVERIFY(stopButton2 != nullptr);
    QVERIFY(irButton3 != nullptr);

    QTest::mouseClick(startButton1, Qt::LeftButton);
    QTest::mouseClick(stopButton2, Qt::LeftButton);
    QTest::mouseClick(irButton3, Qt::LeftButton);

    QCOMPARE(commandSpy.count(), 3);
    QCOMPARE(commandSpy.at(0).at(0).toUInt(), 1u);
    QCOMPARE(commandSpy.at(0).at(1).toInt(), 0);
    QCOMPARE(commandSpy.at(0).at(2).toBool(), true);
    QCOMPARE(commandSpy.at(0).at(3).toDouble(), 4.2);

    QCOMPARE(commandSpy.at(1).at(0).toUInt(), 2u);
    QCOMPARE(commandSpy.at(1).at(1).toInt(), 0);
    QCOMPARE(commandSpy.at(1).at(2).toBool(), false);
    QCOMPARE(commandSpy.at(1).at(3).toDouble(), 0.0);

    QCOMPARE(commandSpy.at(2).at(0).toUInt(), 3u);
    QCOMPARE(commandSpy.at(2).at(1).toInt(), 2);
    QCOMPARE(commandSpy.at(2).at(2).toBool(), true);
    QCOMPARE(commandSpy.at(2).at(3).toDouble(), 4.2);
}

void WidgetTests::chargerStatusWidget_detailedControlsEmitSelectedCommandAndCollectHistory()
{
    ChargerStatusWidget widget;
    QSignalSpy commandSpy(&widget, &ChargerStatusWidget::commandRequested);

    auto* detailedPlot = widget.findChild<ChargerHistoryPlotWidget*>("detailedHistoryPlot2");
    auto* chargerTabs = widget.findChild<QTabWidget*>("detailedChargerTabWidget");
    auto* modeCcButton = widget.findChild<QPushButton*>("detailedModeCcButton");
    auto* modeCvButton = widget.findChild<QPushButton*>("detailedModeCvButton");
    auto* setpointSpin = widget.findChild<QDoubleSpinBox*>("detailedSetpointSpinBox");
    auto* startButton = widget.findChild<QPushButton*>("detailedStartButton");
    auto* stopButton = widget.findChild<QPushButton*>("detailedStopButton");

    QVERIFY(detailedPlot != nullptr);
    QVERIFY(chargerTabs != nullptr);
    QVERIFY(modeCcButton != nullptr);
    QVERIFY(modeCvButton != nullptr);
    QVERIFY(setpointSpin != nullptr);
    QVERIFY(startButton != nullptr);
    QVERIFY(stopButton != nullptr);

    chargerTabs->setCurrentIndex(1);
    QTest::mouseClick(modeCvButton, Qt::LeftButton);
    setpointSpin->setValue(4.175);
    QTest::mouseClick(startButton, Qt::LeftButton);

    QCOMPARE(commandSpy.count(), 1);
    const QList<QVariant> startCommand = commandSpy.takeFirst();
    QCOMPARE(startCommand.at(0).toUInt(), 2u);
    QCOMPARE(startCommand.at(1).toInt(), 1);
    QCOMPARE(startCommand.at(2).toBool(), true);
    QCOMPARE(startCommand.at(3).toDouble(), 4.175);

    Logger::instance().logDecoded(
        "PCP status | DEV=1 | NAME=Charger Bus | MSG=18\n"
        "  charger_id = 2 (raw=2)\n"
        "  current = 1.5 (raw=1500)\n"
        "  status = 1 (raw=1)\n"
        "  temperature = 26 (raw=260)\n"
        "  voltage = 4.12 (raw=4120)");
    QCoreApplication::processEvents();
    QVERIFY(detailedPlot->sampleCountForCharger(2) >= 1);
    QCOMPARE(detailedPlot->selectedCharger(), 2u);

    QTest::mouseClick(stopButton, Qt::LeftButton);
    QCOMPARE(commandSpy.count(), 1);
    const QList<QVariant> stopCommand = commandSpy.takeFirst();
    QCOMPARE(stopCommand.at(0).toUInt(), 2u);
    QCOMPARE(stopCommand.at(1).toInt(), 1);
    QCOMPARE(stopCommand.at(2).toBool(), false);
    QCOMPARE(stopCommand.at(3).toDouble(), 4.175);
}

void WidgetTests::chargerStatusWidget_updatesFromLoggerTraffic()
{
    ChargerStatusWidget widget;

    auto* indicator = widget.findChild<ComsActivityWidget*>("activityIndicator");
    auto* slot1Label = widget.findChild<QLabel*>("slotStatusLabel1");
    auto* slot2Label = widget.findChild<QLabel*>("slotStatusLabel2");
    auto* slot3Label = widget.findChild<QLabel*>("slotStatusLabel3");
    auto* generalVoltage1 = widget.findChild<QLabel*>("generalVoltageLabel1");
    auto* generalCurrent1 = widget.findChild<QLabel*>("generalCurrentLabel1");
    auto* generalTemp1 = widget.findChild<QLabel*>("generalTempLabel1");
    auto* generalTitle1 = widget.findChild<QLabel*>("generalTitleLabel1");
    auto* generalVoltage2 = widget.findChild<QLabel*>("generalVoltageLabel2");
    auto* generalCurrent2 = widget.findChild<QLabel*>("generalCurrentLabel2");
    auto* generalTemp2 = widget.findChild<QLabel*>("generalTempLabel2");
    auto* generalTitle2 = widget.findChild<QLabel*>("generalTitleLabel2");
    auto* generalVoltage3 = widget.findChild<QLabel*>("generalVoltageLabel3");
    auto* generalCurrent3 = widget.findChild<QLabel*>("generalCurrentLabel3");
    auto* generalTemp3 = widget.findChild<QLabel*>("generalTempLabel3");
    auto* generalTitle3 = widget.findChild<QLabel*>("generalTitleLabel3");

    QVERIFY(indicator != nullptr);
    QVERIFY(slot1Label != nullptr);
    QVERIFY(slot2Label != nullptr);
    QVERIFY(slot3Label != nullptr);
    QVERIFY(generalVoltage1 != nullptr);
    QVERIFY(generalCurrent1 != nullptr);
    QVERIFY(generalTemp1 != nullptr);
    QVERIFY(generalTitle1 != nullptr);
    QVERIFY(generalVoltage2 != nullptr);
    QVERIFY(generalCurrent2 != nullptr);
    QVERIFY(generalTemp2 != nullptr);
    QVERIFY(generalTitle2 != nullptr);
    QVERIFY(generalVoltage3 != nullptr);
    QVERIFY(generalCurrent3 != nullptr);
    QVERIFY(generalTemp3 != nullptr);
    QVERIFY(generalTitle3 != nullptr);
    QVERIFY(!indicator->isSlotFresh(0));
    QVERIFY(!indicator->isSlotFresh(1));
    QVERIFY(!indicator->isSlotFresh(2));
    QVERIFY(slot1Label->text().contains("Voltage"));
    QVERIFY(slot1Label->text().contains("--"));
    QVERIFY(slot2Label->text().contains("Current"));
    QVERIFY(slot3Label->text().contains("Status"));

    Logger::instance().logDecoded(
        "PCP status | DEV=1 | NAME=Charger Bus | MSG=18\n"
        "  charger_id = 1 (raw=1)\n"
        "  current = 3.2 (raw=3200)\n"
        "  status = 1 (raw=1)\n"
        "  temperature = 31 (raw=310)\n"
        "  voltage = 12.7 (raw=12700)");
    QCoreApplication::processEvents();

    QVERIFY(indicator->isSlotFresh(0));
    QVERIFY(slot1Label->text().contains("12.7 V"));
    QVERIFY(slot1Label->text().contains("3.2 A"));
    QVERIFY(slot1Label->text().contains("31 C"));
    QVERIFY(slot1Label->text().contains(">Charging<"));
    QCOMPARE(generalVoltage1->text(), QString("Voltage: 12.7 V"));
    QCOMPARE(generalCurrent1->text(), QString("Current: 3.2 A"));
    QCOMPARE(generalTemp1->text(), QString("Temp: 31 C"));
    QCOMPARE(generalTitle1->text(), QString("Charger 1"));

    Logger::instance().logDecoded(
        "PCP status | DEV=1 | NAME=Charger Bus | MSG=18\n"
        "  charger_id = 2 (raw=2)\n"
        "  current = 4.1 (raw=4100)\n"
        "  status = 0 (raw=0)\n"
        "  temperature = 29 (raw=290)\n"
        "  voltage = 12.9 (raw=12900)");
    QCoreApplication::processEvents();

    QVERIFY(indicator->isSlotFresh(1));
    QVERIFY(slot2Label->text().contains("12.9 V"));
    QVERIFY(slot2Label->text().contains("4.1 A"));
    QVERIFY(slot2Label->text().contains("29 C"));
    QVERIFY(slot2Label->text().contains(">Idle<"));
    QCOMPARE(generalVoltage2->text(), QString("Voltage: 12.9 V"));
    QCOMPARE(generalCurrent2->text(), QString("Current: 4.1 A"));
    QCOMPARE(generalTemp2->text(), QString("Temp: 29 C"));
    QCOMPARE(generalTitle2->text(), QString("Charger 2"));

    Logger::instance().logDecoded(
        "PCP status | DEV=1 | NAME=Charger Bus | MSG=18\n"
        "  charger_id = 3 (raw=3)\n"
        "  fault_code = 0 (raw=0)\n"
        "  current = 5.0 (raw=5000)\n"
        "  status = 2 (raw=2)\n"
        "  temperature = 27 (raw=270)\n"
        "  voltage = 13.1 (raw=13100)");
    QCoreApplication::processEvents();

    QVERIFY(indicator->isSlotFresh(2));
    QVERIFY(slot3Label->text().contains("13.1 V"));
    QVERIFY(slot3Label->text().contains("5.0 A"));
    QVERIFY(slot3Label->text().contains("27 C"));
    QVERIFY(slot3Label->text().contains(">Fault 0<"));
    QCOMPARE(generalVoltage3->text(), QString("Voltage: 13.1 V"));
    QCOMPARE(generalCurrent3->text(), QString("Current: 5.0 A"));
    QCOMPARE(generalTemp3->text(), QString("Temp: 27 C"));
    QCOMPARE(generalTitle3->text(), QString("Charger 3"));

    Logger::instance().logDecoded(
        "PCP status | DEV=1 | NAME=Charger Bus | MSG=18\n"
        "  charger_id = 4 (raw=4)\n"
        "  current = 6.0 (raw=6000)\n"
        "  status = 2 (raw=2)\n"
        "  temperature = 35 (raw=350)\n"
        "  voltage = 13.3 (raw=13300)");
    QCoreApplication::processEvents();

    QVERIFY(slot1Label->text().contains("12.7 V"));
    QVERIFY(slot2Label->text().contains("12.9 V"));
    QVERIFY(slot3Label->text().contains("13.1 V"));

    QTest::qWait(550);
    QVERIFY(!indicator->isSlotFresh(0));
    QVERIFY(!indicator->isSlotFresh(1));
    QVERIFY(!indicator->isSlotFresh(2));

    Logger::instance().logStatus("COMS: simulated backend disconnected");
    QCoreApplication::processEvents();
    QVERIFY(slot1Label->text().contains("--"));
    QVERIFY(slot2Label->text().contains("--"));
    QVERIFY(slot3Label->text().contains("--"));
    QCOMPARE(generalVoltage1->text(), QString("Voltage: --"));
    QCOMPARE(generalCurrent1->text(), QString("Current: --"));
    QCOMPARE(generalTemp1->text(), QString("Temp: --"));
}

void WidgetTests::chargerStatusWidget_ignoresInvalidDecodedMessagesAndExtraDevices()
{
    ChargerStatusWidget widget;

    auto* generalTitle1 = widget.findChild<QLabel*>("generalTitleLabel1");
    auto* generalTitle2 = widget.findChild<QLabel*>("generalTitleLabel2");
    auto* generalTitle3 = widget.findChild<QLabel*>("generalTitleLabel3");
    auto* slot1Label = widget.findChild<QLabel*>("slotStatusLabel1");
    auto* slot2Label = widget.findChild<QLabel*>("slotStatusLabel2");
    auto* slot3Label = widget.findChild<QLabel*>("slotStatusLabel3");
    auto* activityIndicator = widget.findChild<ComsActivityWidget*>("activityIndicator");

    QVERIFY(generalTitle1 != nullptr);
    QVERIFY(generalTitle2 != nullptr);
    QVERIFY(generalTitle3 != nullptr);
    QVERIFY(slot1Label != nullptr);
    QVERIFY(slot2Label != nullptr);
    QVERIFY(slot3Label != nullptr);
    QVERIFY(activityIndicator != nullptr);

    Logger::instance().logDecoded("malformed message without device id");
    Logger::instance().logStatus("plain status without coms prefix");
    Logger::instance().logDecoded("PCP status | DEV=abc | NAME=Broken | MSG=18");
    Logger::instance().logDecoded("PCP status | DEV=9999999999999999999999 | NAME=Broken | MSG=18");
    QCoreApplication::processEvents();

    QCOMPARE(generalTitle1->text(), QString("Charger 1"));
    QVERIFY(slot1Label->text().contains("--"));

    Logger::instance().logDecoded(
        "PCP status | DEV=1 | NAME=Node 1 | MSG=18\n"
        "  charger_id = 1 (raw=1)\n"
        "  volt = 12.1 (raw=12100)\n"
        "  temp = 24 (raw=240)\n"
        "  status = 1 (raw=1)");
    Logger::instance().logDecoded(
        "PCP status | DEV=1 | NAME=Node 1 | MSG=18\n"
        "  charger_id = 2 (raw=2)\n"
        "  voltage = 12.2 (raw=12200)\n"
        "  current = 2.2 (raw=2200)\n"
        "  temperature = 25 (raw=250)\n"
        "  status = 2 (raw=2)");
    Logger::instance().logDecoded(
        "PCP status | DEV=1 | NAME=Node 1 | MSG=18\n"
        "  charger_id = 3 (raw=3)\n"
        "  voltage = 12.3 (raw=12300)\n"
        "  current = 3.3 (raw=3300)\n"
        "  temperature = 26 (raw=260)\n"
        "  status = 0 (raw=0)");
    QCoreApplication::processEvents();

    QCOMPARE(generalTitle1->text(), QString("Charger 1"));
    QCOMPARE(generalTitle2->text(), QString("Charger 2"));
    QCOMPARE(generalTitle3->text(), QString("Charger 3"));
    QVERIFY(activityIndicator->isSlotFresh(0));
    QVERIFY(activityIndicator->isSlotFresh(1));
    QVERIFY(activityIndicator->isSlotFresh(2));

    const QString slot1StatusBeforeCellInfo = slot1Label->text();
    Logger::instance().logDecoded(
        "PCP cell_info | DEV=1 | NAME=Node 1 | MSG=19\n"
        "  slot_id = 1 (raw=1)\n"
        "  cell_voltage = 4.024 (raw=4024)\n"
        "  cell_current = -0.325 (raw=-325)\n"
        "  cell_temp = 24.475 (raw=24475)");
    QCoreApplication::processEvents();
    QCOMPARE(slot1Label->text(), slot1StatusBeforeCellInfo);

    Logger::instance().logDecoded(
        "PCP status | DEV=1 | NAME=Node 1 | MSG=18\n"
        "  charger_id = 1 (raw=1)\n"
        "  voltage = 12.9 (raw=12900)\n"
        "  current = 2.9 (raw=2900)\n"
        "  temperature = 29 (raw=290)\n"
        "  status = 1 (raw=1)");
    QCoreApplication::processEvents();
    QVERIFY(slot1Label->text().contains("12.9 V"));
    QCOMPARE(generalTitle1->text(), QString("Charger 1"));

    const QString slot1Before = slot1Label->text();
    const QString slot2Before = slot2Label->text();
    const QString slot3Before = slot3Label->text();

    Logger::instance().logDecoded(
        "PCP status | DEV=1 | NAME=Node 1 | MSG=18\n"
        "  charger_id = 4 (raw=4)\n"
        "  voltage = 13.4 (raw=13400)\n"
        "  current = 4.4 (raw=4400)\n"
        "  temperature = 27 (raw=270)\n"
        "  status = 2 (raw=2)");
    Logger::instance().logStatus("COMS: connected and healthy");
    QCoreApplication::processEvents();

    QCOMPARE(slot1Label->text(), slot1Before);
    QCOMPARE(slot2Label->text(), slot2Before);
    QCOMPARE(slot3Label->text(), slot3Before);

    Logger::instance().logStatus("COMS: no devices discovered");
    QCoreApplication::processEvents();
    QCOMPARE(generalTitle1->text(), QString("Charger 1"));
    QCOMPARE(generalTitle2->text(), QString("Charger 2"));
    QCOMPARE(generalTitle3->text(), QString("Charger 3"));
}

void WidgetTests::helpDialog_initializesExpectedWindowState()
{
    HelpDialog dialog;

    QCOMPARE(dialog.windowTitle(), QString("CellCharger Help"));
    QCOMPARE(dialog.minimumWidth(), 520);
    QCOMPARE(dialog.minimumHeight(), 720);
    QCOMPARE(dialog.maximumWidth(), 520);
    QCOMPARE(dialog.maximumHeight(), 720);
}

void WidgetTests::helpDialog_populatesProjectMetadata()
{
    HelpDialog dialog;

    auto* projectLabel = dialog.findChild<QLabel*>("valueProject");
    auto* versionLabel = dialog.findChild<QLabel*>("valueVersion");
    auto* buildLabel = dialog.findChild<QLabel*>("valueBuild");

    QVERIFY(projectLabel != nullptr);
    QVERIFY(versionLabel != nullptr);
    QVERIFY(buildLabel != nullptr);

    QCOMPARE(projectLabel->text(), QString("CellCharger"));
    QCOMPARE(versionLabel->text(), QString("1.0"));
    QVERIFY(!buildLabel->text().isEmpty());
    QVERIFY(buildLabel->text() != QString("--"));
}

void WidgetTests::consoleWidget_appendsPausesFiltersAndClearsMessages()
{
    PCPDatabase database;
    QVERIFY(database.loadFromFile("pcp.yaml"));
    ConsoleWidget widget(&database);

    auto* pauseButton = widget.findChild<QPushButton*>("pauseButton");
    auto* clearButton = widget.findChild<QPushButton*>("clearButton");
    auto* consoleTable = widget.findChild<QTableView*>("consoleTableView");
    QVERIFY(pauseButton != nullptr);
    QVERIFY(clearButton != nullptr);
    QVERIFY(consoleTable != nullptr);

    const QString statusLine =
        "[12:00:00] RX | Charger Bus (1) | status | Message 18 | DLC 8 | 01 02";
    const QString cellLine =
        "[12:00:01] TX | Charger Bus (1) | cell_info | Message 19 | DLC 8 | 03 04";

    widget.appendMessage("not a formatted PCP line");
    QCOMPARE(widget.m_messageRecords.size(), 0);
    QCOMPARE(widget.m_allMessages.size(), 1);

    widget.appendMessage(statusLine);
    QCOMPARE(widget.m_messageRecords.size(), 1);
    QCOMPARE(widget.extractDeviceName(statusLine), QString("Charger Bus (1)"));
    QCOMPARE(widget.extractMessageName(statusLine), QString("status"));
    QCOMPARE(widget.extractMessageId(statusLine), QString("18"));

    pauseButton->click();
    QVERIFY(widget.m_paused);
    QCOMPARE(pauseButton->text(), QString("Resume"));
    widget.appendMessage(cellLine);
    QCOMPARE(widget.m_messageRecords.size(), 2);
    QCOMPARE(widget.m_pausedBuffer.size(), 1);

    pauseButton->click();
    QVERIFY(!widget.m_paused);
    QCOMPARE(widget.m_pausedBuffer.size(), 0);
    QCOMPARE(pauseButton->text(), QString("Pause"));

    clearButton->click();
    QCOMPARE(widget.m_messageRecords.size(), 0);
    QCOMPARE(widget.m_allMessages.size(), 0);
    QCOMPARE(widget.m_messageModel->rowCount(), 0);
}

void WidgetTests::consoleWidget_watchesDecodedSignalsAndManagesFilters()
{
    PCPDatabase database;
    QVERIFY(database.loadFromFile("pcp.yaml"));
    ConsoleWidget widget(&database);

    auto* deviceCombo = widget.findChild<QComboBox*>("deviceComboBox");
    auto* messageCombo = widget.findChild<QComboBox*>("messageComboBox");
    auto* signalCombo = widget.findChild<QComboBox*>("signalComboBox");
    auto* signalsTable = widget.findChild<QTableWidget*>("signalsTable");
    auto* filterTypeCombo = widget.findChild<QComboBox*>("filterTypeComboBox");
    auto* filterValueEdit = widget.findChild<QLineEdit*>("filterValueEdit");
    auto* addFilterButton = widget.findChild<QPushButton*>("addFilterButton");
    auto* removeFilterButton = widget.findChild<QPushButton*>("removeFilterButton");
    auto* filtersTable = widget.findChild<QTableWidget*>("filtersTable");
    QVERIFY(deviceCombo != nullptr);
    QVERIFY(messageCombo != nullptr);
    QVERIFY(signalCombo != nullptr);
    QVERIFY(signalsTable != nullptr);
    QVERIFY(filterTypeCombo != nullptr);
    QVERIFY(filterValueEdit != nullptr);
    QVERIFY(addFilterButton != nullptr);
    QVERIFY(removeFilterButton != nullptr);
    QVERIFY(filtersTable != nullptr);

    deviceCombo->setCurrentIndex(deviceCombo->findText("Charger Bus (1)"));
    messageCombo->setCurrentIndex(messageCombo->findText("status"));
    signalCombo->setCurrentIndex(signalCombo->findText("voltage"));
    widget.onAddSignalClicked();
    QCOMPARE(signalsTable->rowCount(), 1);
    QCOMPARE(signalsTable->item(0, 3)->text(), QString("-"));

    widget.onStatusMessage(
        "PCP status | DEV=1 | NAME=Charger Bus | MSG=18\n"
        "  voltage = 4.200 (raw=4200)\n"
        "  current = 1.500 (raw=1500)");
    QCOMPARE(signalsTable->item(0, 3)->text(), QString("4.200"));

    widget.onAddSignalClicked();
    QCOMPARE(signalsTable->rowCount(), 1);
    signalsTable->selectRow(0);
    widget.onRemoveSignalClicked();
    QCOMPARE(signalsTable->rowCount(), 0);

    filterTypeCombo->setCurrentText("Text");
    filterValueEdit->setText("status");
    addFilterButton->click();
    QCOMPARE(filtersTable->rowCount(), 1);
    QVERIFY(widget.m_textFilters.contains("status"));

    filterValueEdit->setText("status");
    addFilterButton->click();
    QCOMPARE(filtersTable->rowCount(), 1);

    filtersTable->selectRow(0);
    removeFilterButton->click();
    QCOMPARE(filtersTable->rowCount(), 0);
    QVERIFY(widget.m_textFilters.isEmpty());
}

void WidgetTests::consoleWidget_sendsManualAndPeriodicTxMessages()
{
    PCPDatabase database;
    QVERIFY(database.loadFromFile("pcp.yaml"));
    ConsoleWidget widget(&database);

    auto* txDeviceCombo = widget.findChild<QComboBox*>("txDeviceComboBox");
    auto* txMessageCombo = widget.findChild<QComboBox*>("txMessageComboBox");
    auto* txSignalsTable = widget.findChild<QTableWidget*>("txSignalsTable");
    auto* txPeriodicTable = widget.findChild<QTableWidget*>("txPeriodicTable");
    auto* txIntervalSpin = widget.findChild<QSpinBox*>("txIntervalSpinBox");
    auto* txStatusLabel = widget.findChild<QLabel*>("txStatusLabel");
    auto* sendButton = widget.findChild<QPushButton*>("sendTxButton");
    auto* addPeriodicButton = widget.findChild<QPushButton*>("addPeriodicTxButton");
    auto* startPeriodicButton = widget.findChild<QPushButton*>("startPeriodicTxButton");
    auto* stopPeriodicButton = widget.findChild<QPushButton*>("stopPeriodicTxButton");
    auto* removePeriodicButton = widget.findChild<QPushButton*>("removePeriodicTxButton");
    QVERIFY(txDeviceCombo != nullptr);
    QVERIFY(txMessageCombo != nullptr);
    QVERIFY(txSignalsTable != nullptr);
    QVERIFY(txPeriodicTable != nullptr);
    QVERIFY(txIntervalSpin != nullptr);
    QVERIFY(txStatusLabel != nullptr);
    QVERIFY(sendButton != nullptr);
    QVERIFY(addPeriodicButton != nullptr);
    QVERIFY(startPeriodicButton != nullptr);
    QVERIFY(stopPeriodicButton != nullptr);
    QVERIFY(removePeriodicButton != nullptr);

    sendButton->click();
    QVERIFY(txStatusLabel->text().contains("Select a device and message"));

    txDeviceCombo->setCurrentIndex(txDeviceCombo->findText("Charger Bus (1)"));
    txMessageCombo->setCurrentIndex(txMessageCombo->findText("command"));
    QVERIFY(txSignalsTable->rowCount() > 0);
    txIntervalSpin->setValue(100);

    const int comsHistoryBefore = Logger::instance().comsHistory().size();
    sendButton->click();
    QVERIFY(txStatusLabel->text().contains("Message sent"));
    QVERIFY(Logger::instance().comsHistory().size() > comsHistoryBefore);
    QVERIFY(Logger::instance().comsHistory().last().contains("TX"));

    addPeriodicButton->click();
    QCOMPARE(txPeriodicTable->rowCount(), 1);
    QCOMPARE(widget.m_periodicTxMessages.size(), 1);

    txPeriodicTable->selectRow(0);
    startPeriodicButton->click();
    QVERIFY(widget.m_periodicTxMessages.front().periodicEnabled);
    QVERIFY(widget.m_txTimer.isActive());

    widget.onTxTimerTimeout();
    QVERIFY(txStatusLabel->text().contains("Periodic message sent"));

    txPeriodicTable->selectRow(0);
    stopPeriodicButton->click();
    QVERIFY(!widget.m_periodicTxMessages.front().periodicEnabled);
    QVERIFY(!widget.m_txTimer.isActive());

    widget.onPeriodicTxRowDoubleClicked(0, 0);
    QVERIFY(txStatusLabel->text().contains("Loaded periodic message"));

    txPeriodicTable->selectRow(0);
    removePeriodicButton->click();
    QCOMPARE(txPeriodicTable->rowCount(), 0);
    QCOMPARE(widget.m_periodicTxMessages.size(), 0);
}

void WidgetTests::logWidget_appendsFiltersPausesAndClearsMessages()
{
    LogWidget widget;

    auto* filterEdit = widget.findChild<QLineEdit*>("filterEdit");
    auto* pauseButton = widget.findChild<QPushButton*>("pauseButton");
    auto* clearButton = widget.findChild<QPushButton*>("clearButton");
    auto* logOutput = widget.findChild<QTextEdit*>("logOutput");

    QVERIFY(filterEdit != nullptr);
    QVERIFY(pauseButton != nullptr);
    QVERIFY(clearButton != nullptr);
    QVERIFY(logOutput != nullptr);

    widget.appendMessage("alpha message");
    QVERIFY(logOutput->toPlainText().contains("alpha message"));

    filterEdit->setText("beta");
    widget.appendMessage("gamma message");
    QVERIFY(!logOutput->toPlainText().contains("gamma message"));

    widget.appendMessage("beta message");
    QVERIFY(logOutput->toPlainText().contains("beta message"));

    pauseButton->click();
    QCOMPARE(pauseButton->text(), QString("Resume"));
    const QString pausedSnapshot = logOutput->toPlainText();
    widget.appendMessage("beta while paused");
    QCOMPARE(logOutput->toPlainText(), pausedSnapshot);

    pauseButton->click();
    QCOMPARE(pauseButton->text(), QString("Pause"));

    clearButton->click();
    QVERIFY(logOutput->toPlainText().isEmpty());
}

void WidgetTests::logWidget_startsWithPausedButtonUnchecked()
{
    LogWidget widget;

    auto* pauseButton = widget.findChild<QPushButton*>("pauseButton");
    QVERIFY(pauseButton != nullptr);
    QVERIFY(!pauseButton->isChecked());
    QCOMPARE(pauseButton->text(), QString("Pause"));
}

void WidgetTests::logWidget_loadsExistingLoggerHistoryOnConstruction()
{
    Logger::instance().logStatus("history message for constructor");

    LogWidget widget;
    auto* logOutput = widget.findChild<QTextEdit*>("logOutput");

    QVERIFY(logOutput != nullptr);
    QVERIFY(logOutput->toPlainText().contains("history message for constructor"));
}

void WidgetTests::panelContainer_setsContentAndEmitsCloseSignal()
{
    PanelContainer container("Panel");
    QSignalSpy closeSpy(&container, &PanelContainer::closeRequested);

    auto* first = new QWidget;
    auto* second = new QWidget;

    container.setContentWidget(first);
    QCOMPARE(container.contentWidget(), first);

    container.setContentWidget(second);
    QCOMPARE(container.contentWidget(), second);
    QVERIFY(first->parent() == nullptr);

    QPushButton* closeButton = nullptr;
    const auto buttons = container.findChildren<QPushButton*>();
    for (QPushButton* button : buttons)
    {
        if (button->toolTip() == "Close panel")
        {
            closeButton = button;
            break;
        }
    }

    QVERIFY(closeButton != nullptr);
    closeButton->click();

    QCOMPARE(closeSpy.count(), 1);
    QCOMPARE(qvariant_cast<PanelContainer*>(closeSpy.at(0).at(0)), &container);
}

void WidgetTests::panelContainer_ignoresNullContentWidgets()
{
    PanelContainer container("Panel");
    QWidget original;

    container.setContentWidget(&original);
    QCOMPARE(container.contentWidget(), &original);

    container.setContentWidget(nullptr);
    QCOMPARE(container.contentWidget(), &original);
}

QTEST_MAIN(WidgetTests)

#include "widget_tests.moc"
