#include <QtTest>

#define protected public
#define private public
#include "coms.h"
#include "coms_controller.h"
#include "logger_backend.h"
#include "pcp_database.h"
#include "profile_setup_widget.h"
#include "simulated_coms_backend.h"
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
    void comsController_handlesInvalidRowsAndUnavailableCommandPaths();
    void simulatedBackend_coversEdgeCasesAndSignalValueGeneration();
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
    connect(&profile, &ProfileSetupWidget::commandRequested, &controller, &ComsController::sendChargerCommand);

    profile.addSetpoint1();
    auto* table = profile.findChild<QTableWidget*>("setpointsTable1");
    QVERIFY(table != nullptr);
    auto* timeSpin = qobject_cast<QDoubleSpinBox*>(table->cellWidget(0, 1));
    auto* voltageSpin = qobject_cast<QDoubleSpinBox*>(table->cellWidget(0, 2));
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

void IntegrationTests::comsController_handlesInvalidRowsAndUnavailableCommandPaths()
{
    Coms nullDbView;
    ComsController nullDbController(&nullDbView, nullptr);
    QSignalSpy statusSpy(&Logger::instance(), &Logger::newStatusMessage);

    QVERIFY(!nullDbController.sendChargerCommand(1, 1, true, 4.2));
    QTRY_VERIFY(statusSpy.count() > 0);
    QVERIFY(Logger::instance().statusHistory().last().contains("no PCP database"));

    PCPDatabase database;
    QVERIFY(database.loadFromFile("pcp.yaml"));

    Coms view;
    ComsController controller(&view, &database);

    controller.onConnectConnectionRequested(-1);
    controller.onDisconnectConnectionRequested(99);
    controller.onRemoveConnectionRequested(42);
    QCOMPARE(controller.m_connections.size(), 0);

    controller.onAddConnectionRequested();
    QCOMPARE(controller.m_connections.size(), 1);
    QVERIFY(!controller.sendChargerCommand(1, 1, true, 4.2));
    QVERIFY(Logger::instance().statusHistory().last().contains("no connected backend"));

    QCOMPARE(controller.transportName(ComsType::Serial), QString("Serial"));
    QCOMPARE(controller.transportName(ComsType::Socket_vcan), QString("Socket vcan"));
    QCOMPARE(controller.transportName(ComsType::Socket_UDP), QString("Socket UDP"));
    QCOMPARE(controller.transportName(ComsType::Socket_TCP), QString("Socket TCP"));

    controller.m_connections.front().state = IComsBackend::State::Connecting;
    controller.refreshView();
    controller.refreshOverallIndicator();

    controller.m_connections.front().state = IComsBackend::State::Error;
    controller.refreshView();
    controller.refreshOverallIndicator();

    controller.onRemoveConnectionRequested(0);
    QCOMPARE(controller.m_connections.size(), 0);
}

void IntegrationTests::simulatedBackend_coversEdgeCasesAndSignalValueGeneration()
{
    SimulatedComsBackend missingDbBackend(nullptr);
    QSignalSpy missingDbErrorSpy(&missingDbBackend, &SimulatedComsBackend::errorOccurred);
    missingDbBackend.connectTransport();
    QCOMPARE(missingDbErrorSpy.count(), 1);

    PCPDatabase emptyDatabase;
    SimulatedComsBackend emptyBackend(&emptyDatabase);
    QSignalSpy emptyErrorSpy(&emptyBackend, &SimulatedComsBackend::errorOccurred);
    emptyBackend.connectTransport();
    QCOMPARE(emptyErrorSpy.count(), 1);
    emptyBackend.generateFakeMessage();

    PCPDatabase database;
    QVERIFY(database.loadFromFile("pcp.yaml"));

    SimulatedComsBackend backend(&database);
    QSignalSpy connectedSpy(&backend, &SimulatedComsBackend::connected);
    QSignalSpy receivedSpy(&backend, &SimulatedComsBackend::messageReceived);
    QSignalSpy sentSpy(&backend, &SimulatedComsBackend::messageSent);
    QSignalSpy decodedSpy(&Logger::instance(), &Logger::newDecodedMessage);

    backend.setConfig(ComsConfig{});
    backend.setType(ComsType::Socket_TCP);
    backend.connectTransport();
    QCOMPARE(connectedSpy.count(), 1);

    backend.setType(ComsType::Socket_UDP);
    QCOMPARE(backend.m_state, IComsBackend::State::Disconnected);

    backend.connectTransport();
    QTRY_COMPARE(backend.m_state, IComsBackend::State::Connected);
    backend.generateFakeMessage();
    QTRY_VERIFY(receivedSpy.count() >= 1);
    QVERIFY(decodedSpy.count() >= 1);

    QVERIFY(backend.fakeValueForSignal("charger_id", 2, 1) == 2.0);
    QVERIFY(backend.fakeValueForSignal("slot_id", 2, 1) == 2.0);
    QVERIFY(backend.fakeValueForSignal("voltage", 2, 1) > 12.0);
    QVERIFY(backend.fakeValueForSignal("volt", 2, 1) > 12.0);
    QVERIFY(backend.fakeValueForSignal("cell_voltage", 2, 1) > 4.0);
    QVERIFY(backend.fakeValueForSignal("current", 2, 1) < 0.0);
    QVERIFY(backend.fakeValueForSignal("cell_current", 2, 1) < 0.0);
    QVERIFY(backend.fakeValueForSignal("temperature", 2, 1) > 25.0);
    QVERIFY(backend.fakeValueForSignal("temp", 2, 1) > 25.0);
    QVERIFY(backend.fakeValueForSignal("cell_temp", 2, 1) > 24.0);
    QCOMPARE(backend.fakeValueForSignal("state", 2, 1), 1.0);
    QCOMPARE(backend.fakeValueForSignal("status", 2, 1), 1.0);
    QCOMPARE(backend.fakeValueForSignal("fault", 2, 8), 2.0);
    QCOMPARE(backend.fakeValueForSignal("fault_code", 2, 8), 2.0);
    QCOMPARE(backend.fakeValueForSignal("enabled", 2, 1), 1.0);
    QCOMPARE(backend.fakeValueForSignal("mode", 2, 5), 1.0);
    QVERIFY(backend.fakeValueForSignal("setpoint", 2, 5) > 4.1);
    QCOMPARE(backend.fakeValueForSignal("start", 2, 1), 1.0);
    QVERIFY(backend.fakeValueForSignal("speed", 2, 5) > 1000.0);
    QVERIFY(backend.fakeValueForSignal("power", 2, 5) > 50.0);
    QCOMPARE(backend.fakeValueForSignal("unknown", 2, 5), 7.0);

    PCPEncoder encoder(&database);
    const PCPFrame frame =
        encoder.encode(1, "command", {{"charger_id", 1.0}, {"mode", 1.0}, {"setpoint", 4.2}, {"start", 1.0}});
    QVERIFY(backend.sendFrame(frame, database));
    QTRY_COMPARE(sentSpy.count(), 1);

    backend.disconnectTransport();
    QCOMPARE(backend.m_state, IComsBackend::State::Disconnected);
}

QTEST_MAIN(IntegrationTests)
#include "integration_tests.moc"
