#include <QtTest>
#include <QFile>
#include <QTemporaryDir>

#include "calibration.h"
#include "data_log_writer.h"
#include "measurement_engine.h"
#include "safety_monitor.h"
#include "test_recipe.h"

#include <algorithm>
#include <limits>

class SoftwareRequirementsTests : public QObject
{
    Q_OBJECT

private slots:
    void capacityAndEnergyIntegrateAtRequiredStep();
    void internalResistancePulseValidationAndComputation();
    void csvAndAlarmLogsContainTraceabilityFields();
    void recipesSupportModesConditionsAndCycleLimit();
    void safetyMonitorBlocksUnsafeCurrentEnable();
    void calibrationAndSelfTestSupportDiagnostics();
};

void SoftwareRequirementsTests::capacityAndEnergyIntegrateAtRequiredStep()
{
    CapacityIntegrator integrator;
    IntegrationState state = integrator.update({0.0, 5.0, 10.0, 25.0, 1.0});
    QVERIFY(state.valid);

    state = integrator.update({0.1, 5.0, 10.0, 25.0, 1.0});
    QVERIFY(state.valid);
    QCOMPARE(state.capacityAh, 10.0 * 0.1 / 3600.0);
    QCOMPARE(state.energyWh, 50.0 * 0.1 / 3600.0);

    state = integrator.update({0.25, 5.0, 10.0, 25.0, 1.0});
    QVERIFY(!state.valid);
    QVERIFY(state.error.contains("integration step"));

    integrator.reset();
    state = integrator.update({1.0, 5.0, 1.0, 25.0, 1.0});
    QVERIFY(state.valid);
    state = integrator.update({0.9, 5.0, 1.0, 25.0, 1.0});
    QVERIFY(!state.valid);
    QVERIFY(state.error.contains("backwards"));

    integrator.reset();
    state = integrator.update({0.0, std::numeric_limits<double>::quiet_NaN(), 1.0, 25.0, 1.0});
    QVERIFY(!state.valid);
    QVERIFY(state.error.contains("non-finite"));
}

void SoftwareRequirementsTests::internalResistancePulseValidationAndComputation()
{
    QString error;
    QVERIFY(InternalResistanceCalculator::pulseParametersValid(5.0, 10.0, &error));
    QVERIFY(InternalResistanceCalculator::pulseParametersValid(100.0, 5000.0, &error));
    QVERIFY(!InternalResistanceCalculator::pulseParametersValid(4.9, 10.0, &error));
    QVERIFY(error.contains("current"));
    QVERIFY(!InternalResistanceCalculator::pulseParametersValid(10.0, 5001.0, &error));
    QVERIFY(error.contains("width"));

    const InternalResistanceResult result = InternalResistanceCalculator::compute(4.200, 4.150, 0.0, 50.0);
    QVERIFY(result.valid);
    QCOMPARE(result.resistanceMilliOhm, 1.0);

    const InternalResistanceResult smallCurrent = InternalResistanceCalculator::compute(4.2, 4.1, 0.0, 1.0);
    QVERIFY(!smallCurrent.valid);
    QVERIFY(smallCurrent.error.contains("too small"));

    const InternalResistanceResult outOfRange = InternalResistanceCalculator::compute(4.2, 4.199, 0.0, 50.0);
    QVERIFY(!outOfRange.valid);
    QVERIFY(outOfRange.error.contains("outside"));
}

void SoftwareRequirementsTests::csvAndAlarmLogsContainTraceabilityFields()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    CsvLogWriter log;
    QString error;
    QVERIFY(!log.append({}, &error));
    QVERIFY(error.contains("not open"));
    QVERIFY(!log.flush(&error));
    QVERIFY(error.contains("not open"));

    CsvLogWriter nestedLog;
    QVERIFY(nestedLog.open(QDir(directory.path() + "/nested/logs"), "cycle", &error));
    QVERIFY(QFile::exists(nestedLog.filePath()));

    QFile blocker(directory.path() + "/blocker");
    QVERIFY(blocker.open(QIODevice::WriteOnly));
    blocker.close();
    CsvLogWriter blockedLog;
    QVERIFY(!blockedLog.open(QDir(blocker.fileName()), "cycle", &error));
    QVERIFY(error.contains("could not create"));

    QVERIFY(log.open(QDir(directory.path()), "cycle", &error));
    QVERIFY(log.append({1234, 7, 3, "charge", {1.2, 4.2, 10.0, 26.0, 1.013}, 0.01, 0.04}, &error));
    QVERIFY(log.flush(&error));

    QFile logFile(log.filePath());
    QVERIFY(logFile.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString logText = QString::fromUtf8(logFile.readAll());
    QVERIFY(logText.contains("timestamp_ms,cycle_index,step_index,step_name"));
    QVERIFY(logText.contains("1234,7,3,charge"));
    QVERIFY(logText.contains("temperature_deg_c,pressure_bar"));

    AlarmLogWriter alarms;
    QVERIFY(!alarms.append({}, &error));
    QVERIFY(error.contains("not open"));
    AlarmLogWriter blockedAlarms;
    QVERIFY(!blockedAlarms.open(QDir(blocker.fileName()), "alarms", &error));
    QVERIFY(error.contains("could not create"));
    QVERIFY(alarms.open(QDir(directory.path()), "alarms", &error));
    QVERIFY(alarms.append({5678, "SAF-04", "temperature limit"}, &error));

    QFile alarmFile(alarms.filePath());
    QVERIFY(alarmFile.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString alarmText = QString::fromUtf8(alarmFile.readAll());
    QVERIFY(alarmText.contains("timestamp_ms,code,cause"));
    QVERIFY(alarmText.contains("5678,SAF-04,temperature limit"));
}

void SoftwareRequirementsTests::recipesSupportModesConditionsAndCycleLimit()
{
    TestRecipe recipe;
    recipe.name = "acceptance";
    recipe.cycles = SystemRequirements::maxCyclesPerScript;

    StopConditions chargeStop;
    chargeStop.voltageEnabled = true;
    chargeStop.voltageV = 4.2;
    recipe.steps.push_back({TestStepType::Charge, SystemRequirements::commandModeCC, 10.0, chargeStop});

    StopConditions dischargeStop;
    dischargeStop.temperatureEnabled = true;
    dischargeStop.temperatureDegC = 40.0;
    dischargeStop.pressureEnabled = true;
    dischargeStop.pressureBar = 1.1;
    recipe.steps.push_back({TestStepType::Discharge, SystemRequirements::commandModeCV, 3.0, dischargeStop});

    StopConditions restStop;
    restStop.timeEnabled = true;
    restStop.timeSeconds = 60.0;
    recipe.steps.push_back({TestStepType::Rest, SystemRequirements::commandModeCR, 0.0, restStop});

    QString error;
    QVERIFY(TestRecipeValidator::validate(recipe, &error));

    recipe.cycles = SystemRequirements::maxCyclesPerScript + 1;
    QVERIFY(!TestRecipeValidator::validate(recipe, &error));
    QVERIFY(error.contains("cycle"));

    recipe.cycles = 1;
    recipe.steps.clear();
    QVERIFY(!TestRecipeValidator::validate(recipe, &error));
    QVERIFY(error.contains("at least"));

    recipe.steps.push_back({TestStepType::Charge, 99, 1.0, {}});
    QVERIFY(!TestRecipeValidator::validate(recipe, &error));
    QVERIFY(error.contains("mode"));

    recipe.steps.clear();
    recipe.steps.push_back({TestStepType::Charge, SystemRequirements::commandModeCC, 101.0, {}});
    QVERIFY(!TestRecipeValidator::validate(recipe, &error));
    QVERIFY(error.contains("setpoint"));

    StopConditions badTemperature;
    badTemperature.temperatureEnabled = true;
    badTemperature.temperatureDegC = 100.0;
    recipe.steps.clear();
    recipe.steps.push_back({TestStepType::Rest, SystemRequirements::commandModeCC, 0.0, badTemperature});
    QVERIFY(!TestRecipeValidator::validate(recipe, &error));
    QVERIFY(error.contains("temperature"));

    StopConditions badPressure;
    badPressure.pressureEnabled = true;
    badPressure.pressureBar = 3.0;
    recipe.steps.clear();
    recipe.steps.push_back({TestStepType::Rest, SystemRequirements::commandModeCC, 0.0, badPressure});
    QVERIFY(!TestRecipeValidator::validate(recipe, &error));
    QVERIFY(error.contains("pressure"));
}

void SoftwareRequirementsTests::safetyMonitorBlocksUnsafeCurrentEnable()
{
    SafetyInputs safe;
    SafetyDecision decision = SafetyMonitor::evaluate(safe);
    QVERIFY(decision.currentEnableAllowed);
    QVERIFY(decision.faultCodes.empty());

    SafetyInputs unsafe;
    unsafe.chamberClosed = false;
    unsafe.emergencyStopActive = true;
    unsafe.voltageV = 6.0;
    unsafe.currentA = 111.0;
    unsafe.dutTemperatureDegC = 70.0;
    unsafe.reversePolarityDetected = true;
    unsafe.selfTestPassed = false;

    decision = SafetyMonitor::evaluate(unsafe);
    QVERIFY(!decision.currentEnableAllowed);
    QVERIFY(std::find(decision.faultCodes.begin(), decision.faultCodes.end(), QString("SAF-01")) !=
            decision.faultCodes.end());
    QVERIFY(std::find(decision.faultCodes.begin(), decision.faultCodes.end(), QString("SAF-06")) !=
            decision.faultCodes.end());
    QVERIFY(std::find(decision.faultCodes.begin(), decision.faultCodes.end(), QString("SAF-07")) !=
            decision.faultCodes.end());
    QVERIFY(std::find(decision.faultCodes.begin(), decision.faultCodes.end(), QString("CAL-06")) !=
            decision.faultCodes.end());
}

void SoftwareRequirementsTests::calibrationAndSelfTestSupportDiagnostics()
{
    QString error;
    const CalibrationCoefficients coefficients = CalibrationModel::fromTwoPoints(10.0, 1.0, 20.0, 3.0, &error);
    QCOMPARE(coefficients.gain, 0.2);
    QCOMPARE(coefficients.offset, -1.0);
    QCOMPARE(CalibrationModel::apply(15.0, coefficients), 2.0);

    const SelfTestResult failed = CalibrationModel::selfTest(false, true, false);
    QVERIFY(!failed.passed);
    QVERIFY(std::find(failed.failedChecks.begin(), failed.failedChecks.end(), QString("ADC")) !=
            failed.failedChecks.end());
    QVERIFY(std::find(failed.failedChecks.begin(), failed.failedChecks.end(), QString("sensor")) !=
            failed.failedChecks.end());

    const SelfTestResult passed = CalibrationModel::selfTest(true, true, true);
    QVERIFY(passed.passed);
    QVERIFY(passed.failedChecks.empty());

    const CalibrationCoefficients invalid = CalibrationModel::fromTwoPoints(10.0, 1.0, 10.0, 2.0, &error);
    QCOMPARE(invalid.gain, 1.0);
    QVERIFY(error.contains("distinct"));
}

QTEST_MAIN(SoftwareRequirementsTests)
#include "software_requirements_tests.moc"
