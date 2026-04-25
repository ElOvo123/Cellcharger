#include <QtTest>

#define protected public
#define private public
#include "coms.h"
#include "coms_controller.h"
#include "logger_backend.h"
#include "pcp_database.h"
#include "profile_setup_widget.h"
#undef private
#undef protected

#include <QSignalSpy>
#include <QTableWidget>
#include <QDoubleSpinBox>

class IntegrationTests : public QObject
{
    Q_OBJECT

private slots:
    void comsController_simulatedBackendReceivesAndSendsPcpFrames();
    void profileSetupCommandCanBeSentThroughConnectedComsController();
};

void IntegrationTests::comsController_simulatedBackendReceivesAndSendsPcpFrames()
{
    PCPDatabase database;
    QVERIFY(database.loadFromFile("pcp.yaml"));
    QVERIFY(!database.deviceIds().empty());

    Coms view;
    ComsController controller(&view, &database);
    QSignalSpy statusSpy(&Logger::instance(), &Logger::newStatusMessage);
    QSignalSpy comsSpy(&Logger::instance(), &Logger::newComsMessage);
    QSignalSpy decodedSpy(&Logger::instance(), &Logger::newDecodedMessage);

    controller.onAddConnectionRequested();
    QCOMPARE(controller.m_connections.size(), 1);
    QCOMPARE(controller.m_connections.front().state, IComsBackend::State::Disconnected);

    controller.onConnectConnectionRequested(0);
    QTRY_COMPARE(controller.m_connections.front().state, IComsBackend::State::Connected);
    QTRY_VERIFY(statusSpy.count() > 0);
    QTRY_VERIFY(comsSpy.count() > 0);
    QTRY_VERIFY(decodedSpy.count() > 0);

    QVERIFY(controller.sendChargerCommand(1, 1, true, 4.2));
    QTRY_VERIFY(Logger::instance().comsHistory().join('\n').contains("TX"));

    controller.onDisconnectConnectionRequested(0);
    QTRY_COMPARE(controller.m_connections.front().state, IComsBackend::State::Disconnected);
    controller.onRemoveConnectionRequested(0);
    QCOMPARE(controller.m_connections.size(), 0);
}

void IntegrationTests::profileSetupCommandCanBeSentThroughConnectedComsController()
{
    PCPDatabase database;
    QVERIFY(database.loadFromFile("pcp.yaml"));

    Coms view;
    ComsController controller(&view, &database);
    controller.onAddConnectionRequested();
    controller.onConnectConnectionRequested(0);
    QTRY_COMPARE(controller.m_connections.front().state, IComsBackend::State::Connected);

    ProfileSetupWidget profile;
    QSignalSpy commandSpy(&profile, &ProfileSetupWidget::commandRequested);
    connect(&profile,
            &ProfileSetupWidget::commandRequested,
            &controller,
            &ComsController::sendChargerCommand);

    profile.addSetpoint1();
    auto *table = profile.findChild<QTableWidget*>("setpointsTable1");
    QVERIFY(table != nullptr);
    auto *timeSpin = qobject_cast<QDoubleSpinBox*>(table->cellWidget(0, 1));
    auto *voltageSpin = qobject_cast<QDoubleSpinBox*>(table->cellWidget(0, 2));
    QVERIFY(timeSpin != nullptr);
    QVERIFY(voltageSpin != nullptr);
    timeSpin->setValue(10.0);
    voltageSpin->setValue(4.15);

    const int comsHistoryBefore = Logger::instance().comsHistory().size();
    profile.startTestForSlot(1, table);
    QCOMPARE(commandSpy.count(), 1);
    QTRY_VERIFY(Logger::instance().comsHistory().size() > comsHistoryBefore);
    QVERIFY(Logger::instance().comsHistory().join('\n').contains("TX"));

    profile.stopTestForSlot(1);
    controller.onRemoveConnectionRequested(0);
}

QTEST_MAIN(IntegrationTests)
#include "integration_tests.moc"
