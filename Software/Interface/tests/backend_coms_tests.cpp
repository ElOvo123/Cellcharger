#include <QtTest>

#include "coms_backend.h"
#include "logger_backend.h"

#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>

class BackendComsTests : public QObject
{
    Q_OBJECT

private slots:
    void tcpBackend_publishesReceivedPayloadsToSignalsAndLogger();
    void tcpBackend_reportsCriticalReceiveFailureWhenPeerDropsConnection();
};

void BackendComsTests::tcpBackend_publishesReceivedPayloadsToSignalsAndLogger()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    ComsBackend backend;
    ComsConfig config;
    config.ip = QHostAddress(QHostAddress::LocalHost).toString();
    config.port = static_cast<int>(server.serverPort());

    backend.setType(ComsType::Socket_TCP);
    backend.setConfig(config);

    QSignalSpy connectedSpy(&backend, &ComsBackend::connected);
    QSignalSpy receivedSpy(&backend, &ComsBackend::messageReceived);
    QSignalSpy errorSpy(&backend, &ComsBackend::errorOccurred);
    QSignalSpy loggerSpy(&Logger::instance(), &Logger::newComsMessage);

    backend.connectTransport();

    QCOMPARE(connectedSpy.count(), 1);
    QCOMPARE(errorSpy.count(), 0);
    QVERIFY(server.waitForNewConnection(1000));

    QTcpSocket* peer = server.nextPendingConnection();
    QVERIFY(peer != nullptr);

    const QByteArray payload("RX | Charger Bus (1) | status | Message 18 | DLC 8 | 01 02\n"
                             "RX | Charger Bus (1) | cell_info | Message 19 | DLC 8 | 03 04\n");
    QCOMPARE(peer->write(payload), static_cast<qint64>(payload.size()));
    QVERIFY(peer->waitForBytesWritten(1000));

    QVERIFY(receivedSpy.wait(1000));
    if (receivedSpy.count() < 2)
        QVERIFY(receivedSpy.wait(1000));

    QCOMPARE(receivedSpy.count(), 2);
    QCOMPARE(receivedSpy.at(0).at(0).toString(),
             QString("RX | Charger Bus (1) | status | Message 18 | DLC 8 | 01 02"));
    QCOMPARE(receivedSpy.at(1).at(0).toString(),
             QString("RX | Charger Bus (1) | cell_info | Message 19 | DLC 8 | 03 04"));
    QVERIFY(loggerSpy.count() >= 2);
    QVERIFY(Logger::instance().comsHistory().last().contains("cell_info"));

    backend.disconnectTransport();
}

void BackendComsTests::tcpBackend_reportsCriticalReceiveFailureWhenPeerDropsConnection()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    ComsBackend backend;
    ComsConfig config;
    config.ip = QHostAddress(QHostAddress::LocalHost).toString();
    config.port = static_cast<int>(server.serverPort());

    backend.setType(ComsType::Socket_TCP);
    backend.setConfig(config);

    QSignalSpy stateSpy(&backend, &ComsBackend::stateChanged);
    QSignalSpy errorSpy(&backend, &ComsBackend::errorOccurred);
    QSignalSpy statusSpy(&backend, &ComsBackend::statusMessage);
    QSignalSpy loggerSpy(&Logger::instance(), &Logger::newStatusMessage);

    backend.connectTransport();

    QVERIFY(server.waitForNewConnection(1000));
    QTcpSocket* peer = server.nextPendingConnection();
    QVERIFY(peer != nullptr);

    peer->disconnectFromHost();
    QVERIFY(errorSpy.wait(1000));

    QCOMPARE(errorSpy.count(), 1);
    QVERIFY(errorSpy.at(0).at(0).toString().contains("receive failure"));
    QVERIFY(errorSpy.at(0).at(0).toString().contains("remote host closed"));
    QCOMPARE(statusSpy.last().at(0).toString(), errorSpy.at(0).at(0).toString());
    QVERIFY(loggerSpy.count() >= 1);
    QVERIFY(Logger::instance().statusHistory().last().contains("receive failure"));

    QVERIFY(stateSpy.count() >= 2);
    QCOMPARE(stateSpy.last().at(0).value<IComsBackend::State>(),
             IComsBackend::State::Error);
}

QTEST_MAIN(BackendComsTests)

#include "backend_coms_tests.moc"
