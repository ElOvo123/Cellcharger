#include <QtTest>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QTableWidget>

#include "pcp_database.h"
#include "system_requirements.h"

#define private public
#include "../ui/profile_setup/profile_setup_widget.h"
#undef private

class RequirementsTests : public QObject
{
    Q_OBJECT

private slots:
    void srsDocumentMatchesRequirementRegistry();
    void numericCapabilitiesMeetSrsThresholds();
    void profileSetupControlsExposeSrsRangesAndModes();
    void commandProtocolCanEncodeSrsSetpoints();
};

void RequirementsTests::srsDocumentMatchesRequirementRegistry()
{
    QFile file(QStringLiteral(REQUIREMENTS_DOC_PATH));
    QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.errorString()));
    const QString text = QString::fromUtf8(file.readAll());

    QSet<QString> idsFromDoc;
    QRegularExpression expression("\\|\\s*((?:ELE|MEAS|TEST|ENV|SAF|MECH|CAL|SW|PERF|COMP)-\\d{2})\\s*\\|");
    QRegularExpressionMatchIterator matches = expression.globalMatch(text);
    while (matches.hasNext())
        idsFromDoc.insert(matches.next().captured(1));

    QSet<QString> idsFromCode;
    for (const char* id : SystemRequirements::requirementIds)
        idsFromCode.insert(QString::fromLatin1(id));

    QCOMPARE(idsFromCode.size(), static_cast<int>(SystemRequirements::requirementIds.size()));
    QCOMPARE(idsFromDoc, idsFromCode);
}

void RequirementsTests::numericCapabilitiesMeetSrsThresholds()
{
    QCOMPARE(SystemRequirements::voltageRangeV.min, 0.0);
    QVERIFY(SystemRequirements::voltageRangeV.max >= 5.5);
    QVERIFY(SystemRequirements::currentRangeA.min <= -100.0);
    QVERIFY(SystemRequirements::currentRangeA.max >= 100.0);
    QVERIFY(SystemRequirements::resolution.currentA <= 0.1);
    QVERIFY(SystemRequirements::continuousPowerW >= 550.0);
    QVERIFY(SystemRequirements::resolution.voltageV <= 0.001);
    QVERIFY(SystemRequirements::fullLoadEfficiency >= 0.90);

    QVERIFY(SystemRequirements::integrationUpdateMs <= 100.0);
    QVERIFY(SystemRequirements::pulseCurrentRangeA.min <= 5.0);
    QVERIFY(SystemRequirements::pulseCurrentRangeA.max >= 100.0);
    QVERIFY(SystemRequirements::pulseWidthMs.min <= 10.0);
    QVERIFY(SystemRequirements::pulseWidthMs.max >= 5000.0);
    QVERIFY(SystemRequirements::internalResistanceMilliOhm.min <= 0.1);
    QVERIFY(SystemRequirements::internalResistanceMilliOhm.max >= 100.0);
    QVERIFY(SystemRequirements::irSamplingHz >= 5000.0);
    QVERIFY(SystemRequirements::standardLoggingHz >= 1000.0);
    QVERIFY(SystemRequirements::transientLoggingHz >= 5000.0);

    QVERIFY(SystemRequirements::maxCyclesPerScript >= 10000);
    QVERIFY(SystemRequirements::temperatureRangeDegC.min <= -20.0);
    QVERIFY(SystemRequirements::temperatureRangeDegC.max >= 60.0);
    QVERIFY(SystemRequirements::resolution.temperatureDegC <= 0.1);
    QVERIFY(SystemRequirements::pressureRangeBar.min <= 0.5);
    QVERIFY(SystemRequirements::pressureRangeBar.max >= 2.0);
    QVERIFY(SystemRequirements::resolution.pressureBar <= 0.001);
}

void RequirementsTests::profileSetupControlsExposeSrsRangesAndModes()
{
    ProfileSetupWidget widget;
    widget.addSetpoint1();

    auto* modeCombo = widget.findChild<QComboBox*>("controlModeComboBox1");
    QVERIFY(modeCombo != nullptr);
    QCOMPARE(modeCombo->count(), 4);
    QCOMPARE(modeCombo->itemText(SystemRequirements::commandModeCC), QString("CC"));
    QCOMPARE(modeCombo->itemText(SystemRequirements::commandModeCV), QString("CV"));
    QCOMPARE(modeCombo->itemText(SystemRequirements::commandModeCP), QString("CP"));
    QCOMPARE(modeCombo->itemText(SystemRequirements::commandModeCR), QString("CR"));

    auto* table = widget.findChild<QTableWidget*>("setpointsTable1");
    QVERIFY(table != nullptr);
    QCOMPARE(table->rowCount(), 1);

    auto* voltage = qobject_cast<QDoubleSpinBox*>(table->cellWidget(0, 2));
    auto* current = qobject_cast<QDoubleSpinBox*>(table->cellWidget(0, 3));
    auto* temperature = qobject_cast<QDoubleSpinBox*>(table->cellWidget(0, 4));
    QVERIFY(voltage != nullptr);
    QVERIFY(current != nullptr);
    QVERIFY(temperature != nullptr);

    QCOMPARE(voltage->minimum(), SystemRequirements::voltageRangeV.min);
    QCOMPARE(voltage->maximum(), SystemRequirements::voltageRangeV.max);
    QVERIFY(voltage->singleStep() <= SystemRequirements::resolution.voltageV);
    QCOMPARE(current->minimum(), SystemRequirements::currentRangeA.min);
    QCOMPARE(current->maximum(), SystemRequirements::currentRangeA.max);
    QVERIFY(current->singleStep() <= SystemRequirements::resolution.currentA);
    QCOMPARE(temperature->minimum(), SystemRequirements::temperatureRangeDegC.min);
    QCOMPARE(temperature->maximum(), SystemRequirements::temperatureRangeDegC.max);
    QVERIFY(temperature->singleStep() <= SystemRequirements::resolution.temperatureDegC);
}

void RequirementsTests::commandProtocolCanEncodeSrsSetpoints()
{
    PCPDatabase database;
    QVERIFY(database.loadFromFile("pcp.yaml"));

    const PCPMessageDefinition* command = database.messageByName(1, "command");
    QVERIFY(command != nullptr);
    QCOMPARE(command->dlc, static_cast<uint8_t>(5));

    const auto setpoint = command->signalDefinitions.find("setpoint");
    QVERIFY(setpoint != command->signalDefinitions.end());
    QVERIFY(setpoint->second.isSigned);
    QVERIFY(setpoint->second.scale <= SystemRequirements::resolution.voltageV);

    const double rawMinCurrent = SystemRequirements::currentRangeA.min / setpoint->second.scale;
    const double rawMaxCurrent = SystemRequirements::currentRangeA.max / setpoint->second.scale;
    const double rawMin = -static_cast<double>(1 << (setpoint->second.bitLength - 1));
    const double rawMax = static_cast<double>((1 << (setpoint->second.bitLength - 1)) - 1);
    QVERIFY(rawMinCurrent >= rawMin);
    QVERIFY(rawMaxCurrent <= rawMax);

    const auto mode = command->signalDefinitions.find("mode");
    QVERIFY(mode != command->signalDefinitions.end());
    QVERIFY(mode->second.bitLength >= 2);
}

QTEST_MAIN(RequirementsTests)
#include "requirements_tests.moc"
