#include <QtTest>

#include "coms_activity_widget.h"
#include "charger_history_plot_widget.h"
#include "charger_status_widget.h"
#include "help_dialog.h"
#include "logger_backend.h"
#include "log_widget.h"
#include "panel_container.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QSignalSpy>
#include <QTabWidget>
#include <QTimer>
#include <QTextEdit>

class WidgetTests : public QObject
{
    Q_OBJECT

private slots:
    void comsActivityWidget_turnsGreenOnFreshActivityAndRedAfterTimeout();
    void comsActivityWidget_ignoresInvalidIndexesAndOnlyEmitsOnStateChanges();
    void comsActivityWidget_exposesExpectedSizeHintsAndPaints();
    void chargerStatusWidget_hasOverviewGeneralAndDetailedTabs();
    void chargerStatusWidget_detailedControlsEmitSelectedCommandAndCollectHistory();
    void chargerStatusWidget_updatesFromLoggerTraffic();
    void chargerStatusWidget_ignoresInvalidDecodedMessagesAndExtraDevices();
    void helpDialog_initializesExpectedWindowState();
    void helpDialog_populatesProjectMetadata();
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

    widget.resize(widget.sizeHint());
    QPixmap rendered(widget.size());
    rendered.fill(Qt::transparent);
    widget.render(&rendered);
    QVERIFY(!rendered.isNull());
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
    auto* detailedChargerCombo = widget->findChild<QComboBox*>("detailedChargerComboBox");
    auto* detailedModeCcButton = widget->findChild<QPushButton*>("detailedModeCcButton");
    auto* detailedModeCvButton = widget->findChild<QPushButton*>("detailedModeCvButton");
    auto* detailedSetpointSpin = widget->findChild<QDoubleSpinBox*>("detailedSetpointSpinBox");
    auto* detailedStartButton = widget->findChild<QPushButton*>("detailedStartButton");
    auto* detailedStopButton = widget->findChild<QPushButton*>("detailedStopButton");
    QVERIFY(detailedPlot != nullptr);
    QVERIFY(detailedChargerCombo != nullptr);
    QVERIFY(detailedModeCcButton != nullptr);
    QVERIFY(detailedModeCvButton != nullptr);
    QVERIFY(detailedSetpointSpin != nullptr);
    QVERIFY(detailedStartButton != nullptr);
    QVERIFY(detailedStopButton != nullptr);
    QCOMPARE(detailedChargerCombo->count(), 3);
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

void WidgetTests::chargerStatusWidget_detailedControlsEmitSelectedCommandAndCollectHistory()
{
    ChargerStatusWidget widget;
    QSignalSpy commandSpy(&widget, &ChargerStatusWidget::commandRequested);

    auto* detailedPlot = widget.findChild<ChargerHistoryPlotWidget*>("detailedHistoryPlot");
    auto* chargerCombo = widget.findChild<QComboBox*>("detailedChargerComboBox");
    auto* modeCcButton = widget.findChild<QPushButton*>("detailedModeCcButton");
    auto* modeCvButton = widget.findChild<QPushButton*>("detailedModeCvButton");
    auto* setpointSpin = widget.findChild<QDoubleSpinBox*>("detailedSetpointSpinBox");
    auto* startButton = widget.findChild<QPushButton*>("detailedStartButton");
    auto* stopButton = widget.findChild<QPushButton*>("detailedStopButton");

    QVERIFY(detailedPlot != nullptr);
    QVERIFY(chargerCombo != nullptr);
    QVERIFY(modeCcButton != nullptr);
    QVERIFY(modeCvButton != nullptr);
    QVERIFY(setpointSpin != nullptr);
    QVERIFY(startButton != nullptr);
    QVERIFY(stopButton != nullptr);

    chargerCombo->setCurrentIndex(1);
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
