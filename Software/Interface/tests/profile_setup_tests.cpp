#include <QtTest>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPixmap>
#include <QPushButton>
#include <QSignalSpy>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QWheelEvent>

#define protected public
#define private public
#include "../ui/profile_setup/profile_plot_logic.h"
#include "../ui/profile_setup/profile_plot_widget.h"
#include "../ui/profile_setup/profile_setup_dialogs.h"
#include "../ui/profile_setup/profile_setup_logic.h"
#include "../ui/profile_setup/profile_yaml_logic.h"
#include "../ui/profile_setup/profile_setup_widget.h"
#undef private
#undef protected

#include "logger_backend.h"

class ProfileSetupTests : public QObject
{
    Q_OBJECT

private slots:
    void interpolateLinearReturnsMidpoint();
    void interpolateExponentialReturnsExponentialValue();
    void interpolateRampStepsUpInDiscreteIncrements();
    void paddedBoundsReturnOneUnitMinimumPadding();
    void profilePlotLogic_coversBoundsZoomAndSelectionBranches();
    void renderPlotWidgetWithoutCrashing();
    void plotWidget_handlesDisplayModesMarkersZoomAndSelection();
    void plotWidget_ignoresInputAndHandlesZeroRangeRendering();
    void profileSetupWidget_serializesAndLoadsYaml();
    void profileSetupWidget_tracksActiveStepAndLogsProgress();
    void profileSetupWidget_pauseAndResetControls();
    void profileSetupWidget_helpersCoverInvalidYamlAndFallbacks();
    void profileSetupWidget_exercisesAdditionalSlotBranches();
    void profileSetupLogic_coversNormalizedExecutionBranches();
    void profileYamlLogic_roundTripsAndHandlesPartialDocuments();
    void profileSetupWidget_usesInjectedDialogsForIoAndMessages();
};

class FakeProfileSetupDialogs : public ProfileSetupDialogs
{
public:
    QString savePath;
    QString openPath;
    QString lastTitle;
    QString lastMessage;
    QString lastKind;

    QString getSaveFilePath(QWidget*, const QString&) override
    {
        return savePath;
    }
    QString getOpenFilePath(QWidget*, const QString&) override
    {
        return openPath;
    }
    void showError(QWidget*, const QString& title, const QString& message) override
    {
        lastKind = "error";
        lastTitle = title;
        lastMessage = message;
    }
    void showInfo(QWidget*, const QString& title, const QString& message) override
    {
        lastKind = "info";
        lastTitle = title;
        lastMessage = message;
    }
    void showWarning(QWidget*, const QString& title, const QString& message) override
    {
        lastKind = "warning";
        lastTitle = title;
        lastMessage = message;
    }
};

void ProfileSetupTests::interpolateLinearReturnsMidpoint()
{
    ProfilePlotWidget widget;
    const double value = widget.interpolate(0.0, 0.0, 10.0, 10.0, 5.0, "Linear", 1.0);
    QCOMPARE(value, 5.0);
}

void ProfileSetupTests::interpolateExponentialReturnsExponentialValue()
{
    ProfilePlotWidget widget;
    const double value = widget.interpolate(0.0, 1.0, 10.0, 100.0, 5.0, "Exponential", 1.0);
    QCOMPARE(value, 10.0);
}

void ProfileSetupTests::interpolateRampStepsUpInDiscreteIncrements()
{
    ProfilePlotWidget widget;
    QCOMPARE(widget.interpolate(0.0, 0.0, 10.0, 10.0, 2.4, "Ramp", 3.0), 0.0);
    QCOMPARE(widget.interpolate(0.0, 0.0, 10.0, 10.0, 2.5, "Ramp", 3.0), 3.0);
    QCOMPARE(widget.interpolate(0.0, 0.0, 10.0, 10.0, 9.9, "Ramp", 3.0), 9.0);
    QCOMPARE(widget.interpolate(0.0, 0.0, 10.0, 10.0, 10.0, "Ramp", 3.0), 10.0);
}

void ProfileSetupTests::paddedBoundsReturnOneUnitMinimumPadding()
{
    QCOMPARE(ProfilePlotWidget::paddedLowerBound(0.0, 10.0), -1.0);
    QCOMPARE(ProfilePlotWidget::paddedUpperBound(0.0, 10.0), 11.0);
}

void ProfileSetupTests::profilePlotLogic_coversBoundsZoomAndSelectionBranches()
{
    const QRect plotRect = ProfilePlotLogic::plotRectForSize(640, 320);
    QCOMPARE(plotRect, QRect(70, 20, 550, 240));

    const ProfilePlotBounds emptyBounds = ProfilePlotLogic::computeBounds({}, ProfilePlotWidget::DisplayMode::Voltage);
    QVERIFY(!emptyBounds.hasSetpoints);
    QVERIFY(!emptyBounds.hasDistinctTimeRange);

    const std::vector<Setpoint> sameTime = {{0.0, 4.2, 1.0, 25.0, "Linear", 1.0}, {0.0, 4.1, 2.0, 30.0, "Linear", 1.0}};
    const ProfilePlotBounds sameTimeBounds =
        ProfilePlotLogic::computeBounds(sameTime, ProfilePlotWidget::DisplayMode::Current);
    QVERIFY(sameTimeBounds.hasSetpoints);
    QVERIFY(!sameTimeBounds.hasDistinctTimeRange);

    const std::vector<Setpoint> setpoints = {{5.0, 4.2, 1.0, 25.0, "Ramp", 0.5},
                                             {10.0, 4.0, 2.0, 30.0, "Exponential", 0.5},
                                             {12.0, 4.1, 1.5, 28.0, "Linear", 0.5}};
    const std::vector<Setpoint> normalized = ProfilePlotLogic::normalizedPlotPoints(setpoints);
    QCOMPARE(normalized.size(), size_t(4));
    QCOMPARE(normalized.front().time, 0.0);
    QCOMPARE(normalized.front().current, 0.0);
    QCOMPARE(ProfilePlotLogic::valueForDisplay(setpoints[0], ProfilePlotWidget::DisplayMode::Temperature), 25.0);
    QCOMPARE(ProfilePlotLogic::displayLabel(ProfilePlotWidget::DisplayMode::All), QString("Value"));

    const ProfilePlotBounds bounds = ProfilePlotLogic::computeBounds(setpoints, ProfilePlotWidget::DisplayMode::All);
    QVERIFY(bounds.hasSetpoints);
    QVERIFY(bounds.hasDistinctTimeRange);
    QCOMPARE(bounds.plotPoints.size(), size_t(4));
    QCOMPARE(bounds.displayValues.size(), 12);
    QVERIFY(bounds.maxTime > bounds.minTime);
    QVERIFY(bounds.maxValue > bounds.minValue);

    const ProfilePlotViewState normalizedView = ProfilePlotLogic::normalizedViewState({4.0, 4.0, 2.0, 2.0});
    QCOMPARE(normalizedView.maxTime, 5.0);
    QCOMPARE(normalizedView.maxValue, 3.0);

    const ProfilePlotViewState defaultView = ProfilePlotLogic::defaultViewState(bounds);
    QVERIFY(defaultView.maxTime > defaultView.minTime);
    QVERIFY(defaultView.maxValue > defaultView.minValue);

    QVERIFY(!ProfilePlotLogic::zoomedViewState(plotRect, QPointF(10.0, 10.0), 120, defaultView).has_value());
    const std::optional<ProfilePlotViewState> zoomedIn =
        ProfilePlotLogic::zoomedViewState(plotRect, plotRect.center(), 120, defaultView);
    QVERIFY(zoomedIn.has_value());
    QVERIFY(zoomedIn->maxTime - zoomedIn->minTime < defaultView.maxTime - defaultView.minTime);

    const std::optional<ProfilePlotViewState> zoomedOut =
        ProfilePlotLogic::zoomedViewState(plotRect, plotRect.center(), -120, defaultView);
    QVERIFY(zoomedOut.has_value());
    QVERIFY(zoomedOut->maxTime - zoomedOut->minTime > defaultView.maxTime - defaultView.minTime);

    QVERIFY(!ProfilePlotLogic::selectedViewState(plotRect, QRect(0, 0, 5, 5), defaultView).has_value());
    QVERIFY(!ProfilePlotLogic::selectedViewState(plotRect, QRect(0, 0, 20, 20), defaultView).has_value());

    const std::optional<ProfilePlotViewState> selected = ProfilePlotLogic::selectedViewState(
        plotRect, QRect(plotRect.topLeft() + QPoint(10, 10), plotRect.bottomRight() - QPoint(20, 20)), defaultView);
    QVERIFY(selected.has_value());
    QVERIFY(selected->maxTime > selected->minTime);
    QVERIFY(selected->maxValue > selected->minValue);
}

void ProfileSetupTests::renderPlotWidgetWithoutCrashing()
{
    ProfilePlotWidget widget;
    widget.resize(600, 300);

    std::vector<Setpoint> setpoints = {{0.0, 0.0, 0.0, 25.0, "Linear", 1.0}, {10.0, 10.0, 5.0, 30.0, "Linear", 1.0}};
    widget.setSetpoints(setpoints);
    widget.setDisplayMode(ProfilePlotWidget::DisplayMode::All);

    QPixmap pix(widget.size());
    pix.fill(Qt::transparent);
    widget.render(&pix);

    QVERIFY(!pix.isNull());
    QCOMPARE(pix.size(), widget.size());
}

void ProfileSetupTests::plotWidget_handlesDisplayModesMarkersZoomAndSelection()
{
    ProfilePlotWidget widget;
    widget.resize(640, 320);

    std::vector<Setpoint> setpoints = {{1.0, 4.2, 1.0, 25.0, "Ramp", 0.5},
                                       {3.0, 4.0, 2.0, 30.0, "Exponential", 0.5},
                                       {5.0, 4.1, 1.5, 28.0, "Step", 0.5}};
    widget.setSetpoints(setpoints);
    widget.setDisplayMode(ProfilePlotWidget::DisplayMode::All);
    widget.setActiveStepMarker(true, 2.0, 4.1);
    QCOMPARE(widget.m_activeStepVoltage, 4.1);
    QCOMPARE(widget.m_activeStepCurrent, 4.1);
    QCOMPARE(widget.m_activeStepTemperature, 4.1);

    widget.setActiveStepMarkerValues(true, 2.0, 4.1, 1.8, 27.0);
    QCOMPARE(widget.m_activeStepVoltage, 4.1);
    QCOMPARE(widget.m_activeStepCurrent, 1.8);
    QCOMPARE(widget.m_activeStepTemperature, 27.0);

    QPixmap pix(widget.size());
    pix.fill(Qt::transparent);
    widget.render(&pix);
    QVERIFY(!pix.isNull());
    QVERIFY(widget.m_viewMaxTime > widget.m_viewMinTime);
    QVERIFY(widget.m_viewMaxValue > widget.m_viewMinValue);

    const QRect plotRect(70, 20, widget.width() - 70 - 20, widget.height() - 20 - 60);
    const QPoint center = plotRect.center();
    const QPointF centerF(center);

    const double initialMinTime = widget.m_viewMinTime;
    const double initialMaxTime = widget.m_viewMaxTime;
    QWheelEvent zoomEvent(centerF, centerF, QPoint(), QPoint(0, 120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase,
                          false);
    widget.wheelEvent(&zoomEvent);
    QVERIFY(widget.m_useCustomView);
    QVERIFY(widget.m_viewMaxTime - widget.m_viewMinTime < initialMaxTime - initialMinTime);

    QMouseEvent panPress(QEvent::MouseButtonPress, center, Qt::MiddleButton, Qt::MiddleButton, Qt::NoModifier);
    widget.mousePressEvent(&panPress);
    QVERIFY(widget.m_panning);
    QMouseEvent panMove(QEvent::MouseMove, center + QPoint(20, -10), Qt::NoButton, Qt::MiddleButton, Qt::NoModifier);
    widget.mouseMoveEvent(&panMove);
    QVERIFY(widget.m_useCustomView);
    QMouseEvent panRelease(QEvent::MouseButtonRelease, center + QPoint(20, -10), Qt::MiddleButton, Qt::NoButton,
                           Qt::NoModifier);
    widget.mouseReleaseEvent(&panRelease);
    QVERIFY(!widget.m_panning);

    QMouseEvent selectPress(QEvent::MouseButtonPress, plotRect.topLeft() + QPoint(20, 20), Qt::LeftButton,
                            Qt::LeftButton, Qt::NoModifier);
    widget.mousePressEvent(&selectPress);
    QVERIFY(widget.m_selecting);
    QMouseEvent selectMove(QEvent::MouseMove, plotRect.bottomRight() - QPoint(20, 20), Qt::NoButton, Qt::LeftButton,
                           Qt::NoModifier);
    widget.mouseMoveEvent(&selectMove);
    QVERIFY(!widget.m_selectionRect.isNull());
    QMouseEvent selectRelease(QEvent::MouseButtonRelease, plotRect.bottomRight() - QPoint(20, 20), Qt::LeftButton,
                              Qt::NoButton, Qt::NoModifier);
    widget.mouseReleaseEvent(&selectRelease);
    QVERIFY(!widget.m_selecting);
    QVERIFY(widget.m_useCustomView);

    QMouseEvent resetPress(QEvent::MouseButtonPress, center, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    widget.mousePressEvent(&resetPress);
    QVERIFY(!widget.m_useCustomView);

    widget.setDisplayMode(ProfilePlotWidget::DisplayMode::Current);
    widget.render(&pix);
    widget.setDisplayMode(ProfilePlotWidget::DisplayMode::Temperature);
    widget.render(&pix);
    widget.setDisplayMode(ProfilePlotWidget::DisplayMode::Voltage);
    widget.render(&pix);
}

void ProfileSetupTests::plotWidget_ignoresInputAndHandlesZeroRangeRendering()
{
    ProfilePlotWidget widget;
    widget.resize(640, 320);

    const QRect plotRect = ProfilePlotLogic::plotRectForSize(widget.width(), widget.height());
    const QPoint outside = widget.rect().topLeft() + QPoint(5, 5);
    const QPointF outsideF(outside);

    QWheelEvent ignoredWheel(outsideF, outsideF, QPoint(), QPoint(0, 120), Qt::NoButton, Qt::NoModifier,
                             Qt::NoScrollPhase, false);
    widget.wheelEvent(&ignoredWheel);
    QVERIFY(!widget.m_useCustomView);

    QMouseEvent outsidePress(QEvent::MouseButtonPress, outside, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    widget.mousePressEvent(&outsidePress);
    QVERIFY(!widget.m_selecting);
    QVERIFY(!widget.m_panning);

    widget.setSetpoints({{0.0, 4.2, 1.0, 25.0, "Linear", 1.0}, {0.0, 4.1, 2.0, 30.0, "Linear", 1.0}});

    QPixmap pix(widget.size());
    pix.fill(Qt::transparent);
    widget.render(&pix);
    QVERIFY(!pix.isNull());

    widget.m_useCustomView = true;
    widget.m_viewMinTime = 4.0;
    widget.m_viewMaxTime = 4.0;
    widget.m_viewMinValue = 2.0;
    widget.m_viewMaxValue = 2.0;
    QMouseEvent rightReset(QEvent::MouseButtonPress, plotRect.center(), Qt::RightButton, Qt::RightButton,
                           Qt::NoModifier);
    widget.mousePressEvent(&rightReset);
    QVERIFY(!widget.m_useCustomView);

    widget.setSetpoints({{1.0, 4.2, 1.0, 25.0, "Linear", 1.0}, {2.0, 4.1, 2.0, 30.0, "Linear", 1.0}});
    widget.m_useCustomView = false;
    widget.render(&pix);

    QMouseEvent selectPress(QEvent::MouseButtonPress, plotRect.center(), Qt::LeftButton, Qt::LeftButton,
                            Qt::NoModifier);
    widget.mousePressEvent(&selectPress);
    QVERIFY(widget.m_selecting);
    QMouseEvent tinyRelease(QEvent::MouseButtonRelease, plotRect.center() + QPoint(2, 2), Qt::LeftButton, Qt::NoButton,
                            Qt::NoModifier);
    widget.mouseReleaseEvent(&tinyRelease);
    QVERIFY(!widget.m_selecting);
    QVERIFY(!widget.m_useCustomView);
}

void ProfileSetupTests::profileSetupWidget_serializesAndLoadsYaml()
{
    ProfileSetupWidget widget;

    widget.addSetpoint1();

    auto* table1 = widget.findChild<QTableWidget*>("setpointsTable1");
    QVERIFY(table1 != nullptr);
    QCOMPARE(table1->columnCount(), 7);

    auto* timeSpin = qobject_cast<QDoubleSpinBox*>(table1->cellWidget(0, 1));
    auto* voltageSpin = qobject_cast<QDoubleSpinBox*>(table1->cellWidget(0, 2));
    auto* currentSpin = qobject_cast<QDoubleSpinBox*>(table1->cellWidget(0, 3));
    auto* temperatureSpin = qobject_cast<QDoubleSpinBox*>(table1->cellWidget(0, 4));
    auto* curveCombo = qobject_cast<QComboBox*>(table1->cellWidget(0, 5));
    auto* rampStepSpin = qobject_cast<QDoubleSpinBox*>(table1->cellWidget(0, 6));
    QVERIFY(timeSpin != nullptr);
    QVERIFY(voltageSpin != nullptr);
    QVERIFY(currentSpin != nullptr);
    QVERIFY(temperatureSpin != nullptr);
    QVERIFY(curveCombo != nullptr);
    QVERIFY(rampStepSpin != nullptr);

    timeSpin->setValue(12.0);
    voltageSpin->setValue(4.15);
    currentSpin->setValue(1.75);
    temperatureSpin->setValue(28.5);
    curveCombo->setCurrentText("Exponential");
    rampStepSpin->setValue(0.5);
    auto* profileNameEdit1 = widget.findChild<QLineEdit*>("profileNameEdit1");
    auto* controlModeComboBox1 = widget.findChild<QComboBox*>("controlModeComboBox1");
    QVERIFY(profileNameEdit1 != nullptr);
    QVERIFY(controlModeComboBox1 != nullptr);
    profileNameEdit1->setText("Cycle A");
    controlModeComboBox1->setCurrentIndex(2);

    const QString yaml = widget.serializeProfileToYaml();
    QVERIFY(yaml.contains("slot1:"));
    QVERIFY(yaml.contains("profileName1: Cycle A"));
    QVERIFY(yaml.contains("curveType: Exponential"));

    ProfileSetupWidget restored;
    QString errorMessage;
    QVERIFY(restored.deserializeProfileFromYaml(yaml, &errorMessage));
    QVERIFY2(errorMessage.isEmpty(), qPrintable(errorMessage));

    auto* restoredTable1 = restored.findChild<QTableWidget*>("setpointsTable1");
    auto* restoredProfileNameEdit1 = restored.findChild<QLineEdit*>("profileNameEdit1");
    auto* restoredControlModeComboBox1 = restored.findChild<QComboBox*>("controlModeComboBox1");
    QVERIFY(restoredTable1 != nullptr);
    QVERIFY(restoredProfileNameEdit1 != nullptr);
    QVERIFY(restoredControlModeComboBox1 != nullptr);
    QCOMPARE(restoredTable1->rowCount(), 1);
    QCOMPARE(restoredProfileNameEdit1->text(), QString("Cycle A"));
    QCOMPARE(restoredControlModeComboBox1->currentIndex(), 2);
    QCOMPARE(qobject_cast<QDoubleSpinBox*>(restoredTable1->cellWidget(0, 1))->value(), 12.0);
    QCOMPARE(qobject_cast<QDoubleSpinBox*>(restoredTable1->cellWidget(0, 2))->value(), 4.15);
    QCOMPARE(qobject_cast<QDoubleSpinBox*>(restoredTable1->cellWidget(0, 3))->value(), 1.75);
    QCOMPARE(qobject_cast<QDoubleSpinBox*>(restoredTable1->cellWidget(0, 4))->value(), 28.5);
    QCOMPARE(qobject_cast<QComboBox*>(restoredTable1->cellWidget(0, 5))->currentText(), QString("Exponential"));
    QCOMPARE(qobject_cast<QDoubleSpinBox*>(restoredTable1->cellWidget(0, 6))->value(), 0.5);
}

void ProfileSetupTests::profileSetupWidget_tracksActiveStepAndLogsProgress()
{
    ProfileSetupWidget widget;
    QSignalSpy commandSpy(&widget, &ProfileSetupWidget::commandRequested);
    widget.addSetpoint1();
    widget.addSetpoint1();

    auto* table1 = widget.findChild<QTableWidget*>("setpointsTable1");
    auto* plot1 = widget.m_plotWidget1;
    QVERIFY(table1 != nullptr);
    QVERIFY(plot1 != nullptr);

    auto* controlModeComboBox1 = widget.findChild<QComboBox*>("controlModeComboBox1");
    QVERIFY(controlModeComboBox1 != nullptr);
    controlModeComboBox1->setCurrentIndex(2);
    qobject_cast<QDoubleSpinBox*>(table1->cellWidget(0, 1))->setValue(1.0);
    qobject_cast<QDoubleSpinBox*>(table1->cellWidget(1, 1))->setValue(2.0);
    qobject_cast<QDoubleSpinBox*>(table1->cellWidget(0, 3))->setValue(1.25);
    qobject_cast<QDoubleSpinBox*>(table1->cellWidget(1, 3))->setValue(2.50);

    const int historyBefore = Logger::instance().statusHistory().size();
    widget.startTestForSlot(1, table1);

    QVERIFY(table1->item(0, 0) != nullptr);
    QVERIFY(!table1->item(0, 0)->icon().isNull());
    QVERIFY(table1->item(1, 0)->icon().isNull());
    QVERIFY(plot1->m_activeStepVisible);
    QCOMPARE(plot1->m_activeStepTime, 0.0);
    QCOMPARE(plot1->m_activeStepValue, 0.0);
    QCOMPARE(commandSpy.count(), 1);
    QCOMPARE(commandSpy.at(0).at(0).toUInt(), 1u);
    QCOMPARE(commandSpy.at(0).at(1).toInt(), 0);
    QCOMPARE(commandSpy.at(0).at(2).toBool(), true);
    QCOMPARE(commandSpy.at(0).at(3).toDouble(), 0.0);

    QTest::qWait(1100);

    QVERIFY(!table1->item(0, 0)->icon().isNull());
    QVERIFY(table1->item(1, 0)->icon().isNull());
    QVERIFY(plot1->m_activeStepVisible);
    QCOMPARE(plot1->m_activeStepTime, 1.0);
    QCOMPARE(plot1->m_activeStepValue, 1.25);
    QCOMPARE(plot1->m_activeStepVoltage, 4.2);
    QCOMPARE(plot1->m_activeStepCurrent, 1.25);
    QCOMPARE(plot1->m_activeStepTemperature, 25.0);
    QVERIFY(commandSpy.count() >= 2);
    QCOMPARE(commandSpy.at(1).at(0).toUInt(), 1u);
    QCOMPARE(commandSpy.at(1).at(1).toInt(), 0);
    QCOMPARE(commandSpy.at(1).at(2).toBool(), true);
    QCOMPARE(commandSpy.at(1).at(3).toDouble(), 1.25);

    QTest::qWait(1100);

    QVERIFY(table1->item(0, 0)->icon().isNull());
    QVERIFY(!table1->item(1, 0)->icon().isNull());
    QVERIFY(plot1->m_activeStepVisible);
    QCOMPARE(plot1->m_activeStepTime, 2.0);
    QCOMPARE(plot1->m_activeStepValue, 2.50);
    QCOMPARE(plot1->m_activeStepCurrent, 2.50);
    QVERIFY(commandSpy.count() >= 3);
    QCOMPARE(commandSpy.at(2).at(0).toUInt(), 1u);
    QCOMPARE(commandSpy.at(2).at(1).toInt(), 0);
    QCOMPARE(commandSpy.at(2).at(2).toBool(), true);
    QCOMPARE(commandSpy.at(2).at(3).toDouble(), 2.50);

    const QStringList history = Logger::instance().statusHistory();
    QVERIFY(history.size() >= historyBefore + 2);

    bool sawStep1 = false;
    bool sawStep2 = false;
    for (int i = historyBefore; i < history.size(); ++i)
    {
        sawStep1 = sawStep1 || history.at(i).contains("PROFILE slot 1 step 1/2");
        sawStep2 = sawStep2 || history.at(i).contains("PROFILE slot 1 step 2/2");
    }

    QVERIFY(sawStep1);
    QVERIFY(sawStep2);
}

void ProfileSetupTests::profileSetupWidget_pauseAndResetControls()
{
    ProfileSetupWidget widget;
    QSignalSpy commandSpy(&widget, &ProfileSetupWidget::commandRequested);
    widget.addSetpoint1();
    widget.addSetpoint1();

    auto* table1 = widget.findChild<QTableWidget*>("setpointsTable1");
    auto* pauseButton = widget.findChild<QPushButton*>("pauseTestButton1");
    auto* resetButton = widget.findChild<QPushButton*>("resetTestButton1");
    QVERIFY(table1 != nullptr);
    QVERIFY(pauseButton != nullptr);
    QVERIFY(resetButton != nullptr);

    qobject_cast<QDoubleSpinBox*>(table1->cellWidget(0, 1))->setValue(2.0);
    qobject_cast<QDoubleSpinBox*>(table1->cellWidget(1, 1))->setValue(5.0);
    qobject_cast<QDoubleSpinBox*>(table1->cellWidget(0, 2))->setValue(4.2);
    qobject_cast<QDoubleSpinBox*>(table1->cellWidget(1, 2))->setValue(4.0);
    qobject_cast<QDoubleSpinBox*>(table1->cellWidget(0, 3))->setValue(1.0);
    qobject_cast<QDoubleSpinBox*>(table1->cellWidget(1, 3))->setValue(2.0);
    qobject_cast<QDoubleSpinBox*>(table1->cellWidget(0, 4))->setValue(25.0);
    qobject_cast<QDoubleSpinBox*>(table1->cellWidget(1, 4))->setValue(35.0);

    widget.startTestForSlot(1, table1);
    QVERIFY(widget.m_slotRuntimes[0].timer->isActive());
    QCOMPARE(widget.m_slotRuntimes[0].elapsedSeconds, 0);
    QCOMPARE(widget.m_slotRuntimes[0].activeRow, 0);
    QCOMPARE(pauseButton->text(), QString("Pause"));

    pauseButton->click();
    QVERIFY(widget.m_slotRuntimes[0].paused);
    QVERIFY(!widget.m_slotRuntimes[0].timer->isActive());
    QCOMPARE(pauseButton->text(), QString("Resume"));
    QTest::qWait(1100);
    QCOMPARE(widget.m_slotRuntimes[0].elapsedSeconds, 0);
    QCOMPARE(commandSpy.count(), 1);

    resetButton->click();
    QVERIFY(!widget.m_slotRuntimes[0].paused);
    QVERIFY(!widget.m_slotRuntimes[0].timer->isActive());
    QCOMPARE(widget.m_slotRuntimes[0].elapsedSeconds, 0);
    QCOMPARE(widget.m_slotRuntimes[0].activeRow, 0);
    QCOMPARE(pauseButton->text(), QString("Pause"));
    QVERIFY(widget.m_plotWidget1->m_activeStepVisible);
    QCOMPARE(widget.m_plotWidget1->m_activeStepTime, 0.0);
    QCOMPARE(widget.m_plotWidget1->m_activeStepVoltage, 4.2);
    QCOMPARE(widget.m_plotWidget1->m_activeStepCurrent, 0.0);
    QCOMPARE(widget.m_plotWidget1->m_activeStepTemperature, 25.0);
    QCOMPARE(commandSpy.count(), 2);

    QTest::qWait(1100);
    QCOMPARE(widget.m_slotRuntimes[0].elapsedSeconds, 0);
    QCOMPARE(commandSpy.count(), 2);

    widget.stopTestForSlot(1);
    QVERIFY(!widget.m_slotRuntimes[0].timer->isActive());
    QVERIFY(!widget.m_slotRuntimes[0].paused);
    QCOMPARE(pauseButton->text(), QString("Pause"));
}

void ProfileSetupTests::profileSetupWidget_helpersCoverInvalidYamlAndFallbacks()
{
    ProfileSetupWidget widget;

    QString errorMessage;
    QVERIFY(!widget.deserializeProfileFromYaml("not: [valid", &errorMessage));
    QVERIFY(!errorMessage.isEmpty());

    ProfileDocument partial;
    QVERIFY(ProfileYamlLogic::deserialize("slot1:\n  setpoints:\n    - time: 9\n", partial, &errorMessage));
    QCOMPARE(partial.slotDocuments[0].setpoints.size(), size_t(1));
    QCOMPARE(partial.slotDocuments[0].setpoints.front().curveType, QString("Ramp"));
    QCOMPARE(partial.slotDocuments[0].setpoints.front().rampStep, 1.0);

    auto* controlModeComboBox1 = widget.findChild<QComboBox*>("controlModeComboBox1");
    QVERIFY(controlModeComboBox1 != nullptr);
    controlModeComboBox1->setCurrentIndex(1);
    QCOMPARE(widget.commandModeForSlot(1), 1);
    controlModeComboBox1->setCurrentIndex(3);
    QCOMPARE(widget.commandModeForSlot(1), 0);

    widget.addSetpoint1();
    auto* table1 = widget.findChild<QTableWidget*>("setpointsTable1");
    QVERIFY(table1 != nullptr);
    qobject_cast<QDoubleSpinBox*>(table1->cellWidget(0, 1))->setValue(5.0);
    qobject_cast<QDoubleSpinBox*>(table1->cellWidget(0, 2))->setValue(4.2);
    qobject_cast<QDoubleSpinBox*>(table1->cellWidget(0, 3))->setValue(1.0);

    const std::vector<Setpoint> setpoints = widget.setpointsForTable(table1);
    QCOMPARE(setpoints.size(), size_t(1));
    QCOMPARE(setpoints.front().time, 5.0);
    QCOMPARE(ProfileSetupWidget::displayValueForMode(setpoints.back(), ProfilePlotWidget::DisplayMode::Voltage), 4.2);
    QCOMPARE(ProfileSetupWidget::displayValueForMode(setpoints.back(), ProfilePlotWidget::DisplayMode::Current), 1.0);

    const std::vector<Setpoint> normalized = ProfileSetupLogic::normalizedSetpoints(setpoints);
    QCOMPARE(normalized.size(), size_t(2));
    QCOMPARE(normalized.front().time, 0.0);
    QCOMPARE(normalized.back().time, 5.0);
    QCOMPARE(ProfileSetupWidget::interpolateProfileValue(normalized, 0.0, ProfilePlotWidget::DisplayMode::Current),
             0.0);
    QCOMPARE(ProfileSetupWidget::interpolateProfileValue(normalized, 5.0, ProfilePlotWidget::DisplayMode::Current),
             1.0);
    QVERIFY(ProfileSetupWidget::interpolateProfileValue(normalized, 2.0, ProfilePlotWidget::DisplayMode::Current) >=
            0.0);

    widget.startTestForSlot(1, table1);
    QVERIFY(widget.m_plotWidget1->m_activeStepVisible);
    widget.stopTestForSlot(1);
    QVERIFY(!widget.m_plotWidget1->m_activeStepVisible);
}

void ProfileSetupTests::profileSetupWidget_exercisesAdditionalSlotBranches()
{
    ProfileSetupWidget widget;
    QSignalSpy commandSpy(&widget, &ProfileSetupWidget::commandRequested);

    widget.addSetpoint2();
    widget.addSetpoint3();

    auto* table2 = widget.findChild<QTableWidget*>("setpointsTable2");
    auto* table3 = widget.findChild<QTableWidget*>("setpointsTable3");
    auto* mode2 = widget.findChild<QComboBox*>("controlModeComboBox2");
    auto* mode3 = widget.findChild<QComboBox*>("controlModeComboBox3");
    QVERIFY(table2 != nullptr);
    QVERIFY(table3 != nullptr);
    QVERIFY(mode2 != nullptr);
    QVERIFY(mode3 != nullptr);

    QCOMPARE(widget.plotWidgetForSlot(2), widget.m_plotWidget2);
    QCOMPARE(widget.plotWidgetForSlot(3), widget.m_plotWidget3);
    QCOMPARE(widget.plotWidgetForSlot(99), nullptr);

    qobject_cast<QDoubleSpinBox*>(table2->cellWidget(0, 1))->setValue(3.0);
    qobject_cast<QDoubleSpinBox*>(table2->cellWidget(0, 2))->setValue(4.05);
    qobject_cast<QDoubleSpinBox*>(table2->cellWidget(0, 3))->setValue(1.50);
    mode2->setCurrentIndex(1);
    QCOMPARE(widget.commandModeForSlot(2), 1);
    widget.startTestForSlot(2, table2);
    QVERIFY(widget.m_plotWidget2->m_activeStepVisible);
    QVERIFY(commandSpy.count() >= 1);
    QCOMPARE(commandSpy.takeLast().at(1).toInt(), 1);
    widget.stopTestForSlot(2);
    QVERIFY(!widget.m_plotWidget2->m_activeStepVisible);

    qobject_cast<QDoubleSpinBox*>(table3->cellWidget(0, 1))->setValue(4.0);
    qobject_cast<QDoubleSpinBox*>(table3->cellWidget(0, 2))->setValue(4.15);
    qobject_cast<QDoubleSpinBox*>(table3->cellWidget(0, 3))->setValue(0.75);
    mode3->setCurrentIndex(3);
    QCOMPARE(widget.commandModeForSlot(3), 0);
    widget.startTestForSlot(3, table3);
    QVERIFY(widget.m_plotWidget3->m_activeStepVisible);
    QVERIFY(commandSpy.count() >= 1);
    QCOMPARE(commandSpy.takeLast().at(1).toInt(), 0);
    widget.stopTestForSlot(3);
    QVERIFY(!widget.m_plotWidget3->m_activeStepVisible);

    table2->selectRow(0);
    widget.removeSetpoint2();
    QCOMPARE(table2->rowCount(), 0);
    table3->selectRow(0);
    widget.removeSetpoint3();
    QCOMPARE(table3->rowCount(), 0);
}

void ProfileSetupTests::profileSetupLogic_coversNormalizedExecutionBranches()
{
    std::vector<Setpoint> raw = {{5.0, 4.2, 1.0, 25.0, "Ramp", 0.5},
                                 {10.0, 4.0, 2.0, 30.0, "Exponential", 0.5},
                                 {12.0, 4.1, 1.5, 28.0, "Linear", 0.5}};

    const std::vector<Setpoint> normalized = ProfileSetupLogic::normalizedSetpoints(raw);
    QCOMPARE(normalized.size(), size_t(4));
    QCOMPARE(normalized.front().time, 0.0);
    QCOMPARE(ProfileSetupLogic::normalizedSetpoints({}).size(), size_t(0));
    QCOMPARE(ProfileSetupLogic::normalizedSetpoints({{0.0, 4.0, 1.0, 25.0, "Linear", 1.0}}).size(), size_t(1));
    QCOMPARE(ProfileSetupLogic::displayModeForControlIndex(0), ProfilePlotWidget::DisplayMode::All);
    QCOMPARE(ProfileSetupLogic::displayModeForControlIndex(1), ProfilePlotWidget::DisplayMode::Voltage);
    QCOMPARE(ProfileSetupLogic::displayModeForControlIndex(2), ProfilePlotWidget::DisplayMode::Current);
    QCOMPARE(ProfileSetupLogic::displayModeForControlIndex(3), ProfilePlotWidget::DisplayMode::Temperature);
    QCOMPARE(ProfileSetupLogic::commandModeForControlIndex(1), 1);
    QCOMPARE(ProfileSetupLogic::commandModeForControlIndex(3), 0);
    QCOMPARE(ProfileSetupLogic::activeRowForElapsedSeconds({}, 0), -1);
    QCOMPARE(ProfileSetupLogic::activeRowForElapsedSeconds(raw, 0), 0);
    QCOMPARE(ProfileSetupLogic::activeRowForElapsedSeconds(raw, 11), 2);
    QCOMPARE(ProfileSetupLogic::activeRowForElapsedSeconds(raw, 50), 2);
    QCOMPARE(ProfileSetupLogic::displayValueForMode(raw[0], ProfilePlotWidget::DisplayMode::Current), 1.0);
    QCOMPARE(ProfileSetupLogic::interpolateProfileValue({}, 1.0, ProfilePlotWidget::DisplayMode::Voltage), 0.0);
    QCOMPARE(ProfileSetupLogic::interpolateProfileValue(normalized, -1.0, ProfilePlotWidget::DisplayMode::Voltage),
             normalized.front().voltage);
    QVERIFY(ProfileSetupLogic::interpolateProfileValue(normalized, 6.0, ProfilePlotWidget::DisplayMode::Current) >=
            1.0);
    QCOMPARE(ProfileSetupLogic::interpolateProfileValue(
                 {{0.0, 0.0, 0.0, 25.0, "Linear", 1.0}, {10.0, 5.0, 2.0, 25.0, "Ramp", 0.0}}, 5.0,
                 ProfilePlotWidget::DisplayMode::Current),
             0.0);
    QCOMPARE(ProfileSetupLogic::interpolateProfileValue(
                 {{0.0, 0.0, 0.0, 25.0, "Linear", 1.0}, {10.0, 8.0, 0.0, 25.0, "Exponential", 1.0}}, 5.0,
                 ProfilePlotWidget::DisplayMode::Voltage),
             4.0);

    const ProfileStepState emptyStep = ProfileSetupLogic::stepStateForElapsedSeconds({}, 0, 0);
    QVERIFY(!emptyStep.valid);
    const ProfileStepState step0 = ProfileSetupLogic::stepStateForElapsedSeconds(raw, 0, 2);
    QVERIFY(step0.valid);
    QCOMPARE(step0.activeRow, 0);
    QCOMPARE(step0.commandMode, 0);
    QCOMPARE(step0.markerTime, 0.0);
    QCOMPARE(step0.setpoint, 0.0);
    QVERIFY(!step0.finished);

    const ProfileStepState stepDone = ProfileSetupLogic::stepStateForElapsedSeconds(raw, 12, 1);
    QVERIFY(stepDone.valid);
    QCOMPARE(stepDone.commandMode, 1);
    QVERIFY(stepDone.finished);
    QCOMPARE(stepDone.activeRow, 2);
}

void ProfileSetupTests::profileYamlLogic_roundTripsAndHandlesPartialDocuments()
{
    ProfileDocument document;
    document.slotDocuments[0].profileName = "Alpha";
    document.slotDocuments[0].displayModeIndex = 2;
    document.slotDocuments[0].setpoints.push_back({5.0, 4.2, 1.5, 26.0, "Linear", 0.5});
    document.slotDocuments[1].profileName = "Beta";
    document.slotDocuments[1].displayModeIndex = 1;
    document.slotDocuments[1].setpoints.push_back({10.0, 4.0, 2.0, 30.0, "Ramp", 1.0});

    const QString yaml = ProfileYamlLogic::serialize(document);
    QVERIFY(yaml.contains("profileName1: Alpha"));
    QVERIFY(yaml.contains("displayMode2: 1"));

    ProfileDocument restored;
    QString errorMessage;
    QVERIFY(ProfileYamlLogic::deserialize(yaml, restored, &errorMessage));
    QVERIFY2(errorMessage.isEmpty(), qPrintable(errorMessage));
    QCOMPARE(restored.slotDocuments[0].profileName, QString("Alpha"));
    QCOMPARE(restored.slotDocuments[0].displayModeIndex, 2);
    QCOMPARE(restored.slotDocuments[0].setpoints.size(), size_t(1));
    QCOMPARE(restored.slotDocuments[0].setpoints.front().curveType, QString("Linear"));
    QCOMPARE(restored.slotDocuments[1].profileName, QString("Beta"));
    QCOMPARE(restored.slotDocuments[1].displayModeIndex, 1);
    QCOMPARE(restored.slotDocuments[1].setpoints.front().rampStep, 1.0);
    QCOMPARE(restored.slotDocuments[2].profileName, QString());
    QCOMPARE(restored.slotDocuments[2].displayModeIndex, 0);
    QCOMPARE(restored.slotDocuments[2].setpoints.size(), size_t(0));

    ProfileDocument partial;
    QVERIFY(ProfileYamlLogic::deserialize("slot1:\n  setpoints:\n    - time: 3\n      current: 2\n", partial,
                                          &errorMessage));
    QCOMPARE(partial.slotDocuments[0].setpoints.size(), size_t(1));
    QCOMPARE(partial.slotDocuments[0].setpoints.front().curveType, QString("Ramp"));
    QCOMPARE(partial.slotDocuments[0].setpoints.front().rampStep, 1.0);
    QCOMPARE(partial.slotDocuments[1].setpoints.size(), size_t(0));

    QVERIFY(!ProfileYamlLogic::deserialize("[1, 2, 3]", partial, &errorMessage));
    QVERIFY(!errorMessage.isEmpty());

    ProfileDocument fallbackDocument;
    QVERIFY(ProfileYamlLogic::deserialize("slot1:\n"
                                          "  setpoints:\n"
                                          "    - []\n"
                                          "profileName1: [bad]\n"
                                          "displayMode1: nope\n",
                                          fallbackDocument, &errorMessage));
    QCOMPARE(fallbackDocument.slotDocuments[0].setpoints.size(), size_t(0));
    QCOMPARE(fallbackDocument.slotDocuments[0].profileName, QString());
    QCOMPARE(fallbackDocument.slotDocuments[0].displayModeIndex, 0);
}

void ProfileSetupTests::profileSetupWidget_usesInjectedDialogsForIoAndMessages()
{
    FakeProfileSetupDialogs dialogs;
    ProfileSetupWidget widget(nullptr, &dialogs);
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    widget.saveProfile();
    QVERIFY(dialogs.lastKind.isEmpty());

    widget.loadProfile();
    QVERIFY(dialogs.lastKind.isEmpty());

    dialogs.savePath = "/definitely/missing/path/profile.yaml";
    widget.saveProfile();
    QCOMPARE(dialogs.lastKind, QString("error"));
    QVERIFY(dialogs.lastMessage.contains("Could not open file for writing"));

    dialogs.lastKind.clear();
    dialogs.lastMessage.clear();
    dialogs.openPath = "/definitely/missing/path/profile.yaml";
    widget.loadProfile();
    QCOMPARE(dialogs.lastKind, QString("error"));
    QVERIFY(dialogs.lastMessage.contains("Could not open file for reading"));

    dialogs.lastKind.clear();
    dialogs.lastMessage.clear();
    auto* table1 = widget.findChild<QTableWidget*>("setpointsTable1");
    QVERIFY(table1 != nullptr);
    widget.startTestForSlot(1, table1);
    QCOMPARE(dialogs.lastKind, QString("warning"));
    QVERIFY(dialogs.lastMessage.contains("Please add at least one setpoint"));

    dialogs.lastKind.clear();
    dialogs.lastMessage.clear();
    widget.resetTestForSlot(1);
    QCOMPARE(dialogs.lastKind, QString("warning"));
    QVERIFY(dialogs.lastMessage.contains("Please add at least one setpoint"));

    widget.addSetpoint1();
    dialogs.savePath = tempDir.filePath("profile.yaml");
    dialogs.lastKind.clear();
    dialogs.lastMessage.clear();
    widget.saveProfile();
    QCOMPARE(dialogs.lastKind, QString("info"));
    QVERIFY(dialogs.lastMessage.contains("saved successfully"));

    dialogs.openPath = dialogs.savePath;
    dialogs.lastKind.clear();
    dialogs.lastMessage.clear();
    widget.loadProfile();
    QCOMPARE(dialogs.lastKind, QString("info"));
    QVERIFY(dialogs.lastMessage.contains("loaded successfully"));
}

QTEST_MAIN(ProfileSetupTests)
#include "profile_setup_tests.moc"
