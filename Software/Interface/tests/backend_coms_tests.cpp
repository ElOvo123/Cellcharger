#include <QtTest>

#define private public
#include "coms_backend.h"
#undef private
#include "logger_backend.h"
#include "pcp_database.h"

#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

class BackendComsTests : public QObject
{
    Q_OBJECT

private slots:
    void backend_splitsAndPublishesPayloadLines();
    void backend_receiveFailureTransitionsToErrorOnce();
    void backend_sendFrameLogsAndFailsWhenNoTransportIsOpen();
    void backend_setTypeDisconnectsConnectedTransport();
    void backend_serialPublishesPseudoTerminalTraffic();
    void backend_udpPublishesDatagramsAndSendsFrames();
    void backend_connectionFailuresReportErrors();
    void backend_cleanupClosesOwnedTransports();
    void tcpBackend_publishesReceivedPayloadsToSignalsAndLogger();
    void tcpBackend_reportsCriticalReceiveFailureWhenPeerDropsConnection();
};

void BackendComsTests::backend_splitsAndPublishesPayloadLines()
{
    ComsBackend backend;
    QSignalSpy receivedSpy(&backend, &ComsBackend::messageReceived);
    QSignalSpy loggerSpy(&Logger::instance(), &Logger::newComsMessage);

    backend.publishReceivedPayload("\n RX one \n\n TX two \n");

    QCOMPARE(receivedSpy.count(), 2);
    QCOMPARE(receivedSpy.at(0).at(0).toString(), QString("RX one"));
    QCOMPARE(receivedSpy.at(1).at(0).toString(), QString("TX two"));
    QVERIFY(loggerSpy.count() >= 2);

    backend.publishReceivedPayload("   \n\t");
    QCOMPARE(receivedSpy.count(), 2);
}

void BackendComsTests::backend_receiveFailureTransitionsToErrorOnce()
{
    ComsBackend backend;
    backend.m_state = IComsBackend::State::Connected;

    QSignalSpy stateSpy(&backend, &ComsBackend::stateChanged);
    QSignalSpy errorSpy(&backend, &ComsBackend::errorOccurred);
    QSignalSpy statusSpy(&backend, &ComsBackend::statusMessage);
    QSignalSpy loggerSpy(&Logger::instance(), &Logger::newStatusMessage);

    backend.handleReceiveFailure(QString());
    QCOMPARE(backend.m_state, IComsBackend::State::Error);
    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(errorSpy.count(), 1);
    QVERIFY(errorSpy.at(0).at(0).toString().contains("unknown receive failure"));
    QCOMPARE(statusSpy.at(0).at(0).toString(), errorSpy.at(0).at(0).toString());
    QVERIFY(loggerSpy.count() >= 1);

    backend.handleReceiveFailure("second failure");
    QCOMPARE(errorSpy.count(), 1);

    backend.m_state = IComsBackend::State::Disconnected;
    backend.handleReceiveFailure("ignored while disconnected");
    QCOMPARE(errorSpy.count(), 1);
}

void BackendComsTests::backend_sendFrameLogsAndFailsWhenNoTransportIsOpen()
{
    PCPDatabase database;
    QVERIFY(database.loadFromFile("pcp.yaml"));

    PCPFrame frame;
    frame.id = (1u << database.idLayout().messageIdBits) | 20u;
    frame.dlc = 4;
    frame.data = {1, 1, 0x10, 0x80, 0, 0, 0, 0};

    ComsBackend backend;
    QSignalSpy sentSpy(&backend, &ComsBackend::messageSent);
    QSignalSpy loggerSpy(&Logger::instance(), &Logger::newComsMessage);

    backend.setType(ComsType::Serial);
    QVERIFY(!backend.sendFrame(frame, database));
    backend.setType(ComsType::Socket_TCP);
    QVERIFY(!backend.sendFrame(frame, database));
    backend.setType(ComsType::Socket_UDP);
    QVERIFY(!backend.sendFrame(frame, database));
    backend.setType(ComsType::Socket_vcan);
    QVERIFY(!backend.sendFrame(frame, database));

    int pipeFds[2] = {-1, -1};
    QVERIFY(pipe(pipeFds) == 0);
    backend.m_can_socket = pipeFds[1];
    QVERIFY(backend.sendFrame(frame, database));
    backend.m_can_socket = -1;
    close(pipeFds[0]);
    close(pipeFds[1]);

    backend.setType(ComsType::Socket_UDP);
    backend.m_udp = new QUdpSocket(&backend);
    (void)backend.sendFrame(frame, database);
    delete backend.m_udp;
    backend.m_udp = nullptr;

    QCOMPARE(sentSpy.count(), 6);
    QVERIFY(loggerSpy.count() >= 6);
    QVERIFY(sentSpy.at(0).at(0).toString().contains("TX"));
}

void BackendComsTests::backend_setTypeDisconnectsConnectedTransport()
{
    ComsBackend backend;
    backend.m_state = IComsBackend::State::Connected;

    QSignalSpy disconnectedSpy(&backend, &ComsBackend::disconnected);
    QSignalSpy statusSpy(&backend, &ComsBackend::statusMessage);
    QSignalSpy stateSpy(&backend, &ComsBackend::stateChanged);

    backend.setType(ComsType::Socket_TCP);

    QCOMPARE(backend.m_type, ComsType::Socket_TCP);
    QCOMPARE(backend.m_state, IComsBackend::State::Disconnected);
    QCOMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(statusSpy.last().at(0).toString(), QString("Real backend disconnected"));
    QVERIFY(stateSpy.count() >= 1);
}

void BackendComsTests::backend_serialPublishesPseudoTerminalTraffic()
{
    const int masterFd = posix_openpt(O_RDWR | O_NOCTTY);
    if (masterFd < 0)
        QSKIP("Pseudo-terminal support is unavailable");

    QVERIFY(grantpt(masterFd) == 0);
    QVERIFY(unlockpt(masterFd) == 0);
    char* slaveName = ptsname(masterFd);
    QVERIFY(slaveName != nullptr);

    ComsBackend backend;
    ComsConfig config;
    config.serialPort = QString::fromLocal8Bit(slaveName);
    config.baudrate = 115200;
    backend.setType(ComsType::Serial);
    backend.setConfig(config);

    QSignalSpy connectedSpy(&backend, &ComsBackend::connected);
    QSignalSpy receivedSpy(&backend, &ComsBackend::messageReceived);

    backend.connectTransport();
    QCOMPARE(connectedSpy.count(), 1);
    QCOMPARE(backend.m_state, IComsBackend::State::Connected);
    QVERIFY(backend.m_serial != nullptr);
    QVERIFY(backend.m_serial->isOpen());

    const QByteArray inbound("RX serial line\n");
    QCOMPARE(write(masterFd, inbound.constData(), static_cast<size_t>(inbound.size())),
             static_cast<ssize_t>(inbound.size()));
    QVERIFY(receivedSpy.wait(1000));
    QCOMPARE(receivedSpy.at(0).at(0).toString(), QString("RX serial line"));

    backend.disconnectTransport();
    close(masterFd);
}

void BackendComsTests::backend_udpPublishesDatagramsAndSendsFrames()
{
    PCPDatabase database;
    QVERIFY(database.loadFromFile("pcp.yaml"));

    ComsBackend backend;
    backend.setType(ComsType::Socket_UDP);
    ComsConfig bindConfig;
    bindConfig.port = 0;
    backend.setConfig(bindConfig);

    QSignalSpy connectedSpy(&backend, &ComsBackend::connected);
    backend.connectTransport();
    if (connectedSpy.count() != 1)
        QSKIP("UDP sockets are unavailable in this test environment");

    QCOMPARE(backend.m_state, IComsBackend::State::Connected);
    QVERIFY(backend.m_udp != nullptr);
    QVERIFY(backend.m_udp->localPort() > 0);

    QSignalSpy receivedSpy(&backend, &ComsBackend::messageReceived);
    QSignalSpy loggerSpy(&Logger::instance(), &Logger::newComsMessage);

    QUdpSocket sender;
    const QByteArray inbound("RX udp one\nRX udp two\n");
    QCOMPARE(sender.writeDatagram(inbound, QHostAddress::LocalHost, backend.m_udp->localPort()),
             static_cast<qint64>(inbound.size()));
    QVERIFY(receivedSpy.wait(1000));
    if (receivedSpy.count() < 2)
        QVERIFY(receivedSpy.wait(1000));

    QCOMPARE(receivedSpy.count(), 2);
    QCOMPARE(receivedSpy.at(0).at(0).toString(), QString("RX udp one"));
    QCOMPARE(receivedSpy.at(1).at(0).toString(), QString("RX udp two"));
    QVERIFY(loggerSpy.count() >= 2);

    QUdpSocket receiver;
    QVERIFY(receiver.bind(QHostAddress::LocalHost, 0));

    ComsConfig config;
    config.ip = QHostAddress(QHostAddress::LocalHost).toString();
    config.port = static_cast<int>(receiver.localPort());
    backend.setConfig(config);

    PCPFrame frame;
    frame.id = (1u << database.idLayout().messageIdBits) | 20u;
    frame.dlc = 4;
    frame.data = {1, 1, 0x10, 0x80, 0, 0, 0, 0};

    QVERIFY(backend.sendFrame(frame, database));
    QVERIFY(receiver.waitForReadyRead(1000));
    QVERIFY(receiver.hasPendingDatagrams());

    QByteArray datagram;
    datagram.resize(static_cast<int>(receiver.pendingDatagramSize()));
    receiver.readDatagram(datagram.data(), datagram.size());
    QVERIFY(QString::fromUtf8(datagram).contains("TX"));

    backend.disconnectTransport();
}

void BackendComsTests::backend_connectionFailuresReportErrors()
{
    ComsBackend guardBackend;
    guardBackend.handleSerialReadyRead();
    guardBackend.handleTcpReadyRead();
    guardBackend.handleUdpReadyRead();
    guardBackend.handleTcpDisconnected();
    guardBackend.setState(IComsBackend::State::Disconnected);

    ComsBackend serialBackend;
    serialBackend.setType(ComsType::Serial);
    ComsConfig serialConfig;
    serialConfig.serialPort = "definitely-not-a-real-serial-port";
    serialBackend.setConfig(serialConfig);

    QSignalSpy serialStateSpy(&serialBackend, &ComsBackend::stateChanged);
    QSignalSpy serialErrorSpy(&serialBackend, &ComsBackend::errorOccurred);
    serialBackend.connectTransport();

    QCOMPARE(serialBackend.m_state, IComsBackend::State::Error);
    QCOMPARE(serialErrorSpy.count(), 1);
    QVERIFY(serialStateSpy.count() >= 2);

    ComsBackend canBackend;
    canBackend.setType(ComsType::Socket_vcan);
    ComsConfig canConfig;
    canConfig.canInterface = "definitely-not-a-real-can-interface";
    canBackend.setConfig(canConfig);

    QSignalSpy canErrorSpy(&canBackend, &ComsBackend::errorOccurred);
    canBackend.connectTransport();

    QCOMPARE(canBackend.m_state, IComsBackend::State::Error);
    QCOMPARE(canErrorSpy.count(), 1);
    QCOMPARE(canBackend.m_can_socket, -1);

    canBackend.connectTransport();
    QCOMPARE(canErrorSpy.count(), 1);

    serialBackend.publishReceivedPayload("RX keep\n   \nTX keep");
}

void BackendComsTests::backend_cleanupClosesOwnedTransports()
{
    ComsBackend backend;
    backend.m_serial = new QSerialPort(&backend);
    backend.m_tcp = new QTcpSocket(&backend);
    backend.m_udp = new QUdpSocket(&backend);

    int pipeFds[2] = {-1, -1};
    QVERIFY(pipe(pipeFds) == 0);
    backend.m_can_socket = pipeFds[1];

    backend.cleanup();

    QVERIFY(backend.m_serial == nullptr);
    QVERIFY(backend.m_tcp == nullptr);
    QVERIFY(backend.m_udp == nullptr);
    QCOMPARE(backend.m_can_socket, -1);
    close(pipeFds[0]);
}

void BackendComsTests::tcpBackend_publishesReceivedPayloadsToSignalsAndLogger()
{
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost, 0))
    {
        const QByteArray reason = QString("Loopback TCP server unavailable for integration-style backend test: %1")
                                      .arg(server.errorString())
                                      .toUtf8();
        QSKIP(reason.constData());
    }

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
    QCOMPARE(receivedSpy.at(0).at(0).toString(), QString("RX | Charger Bus (1) | status | Message 18 | DLC 8 | 01 02"));
    QCOMPARE(receivedSpy.at(1).at(0).toString(),
             QString("RX | Charger Bus (1) | cell_info | Message 19 | DLC 8 | 03 04"));
    QVERIFY(loggerSpy.count() >= 2);
    QVERIFY(Logger::instance().comsHistory().last().contains("cell_info"));

    backend.disconnectTransport();
}

void BackendComsTests::tcpBackend_reportsCriticalReceiveFailureWhenPeerDropsConnection()
{
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost, 0))
    {
        const QByteArray reason = QString("Loopback TCP server unavailable for integration-style backend test: %1")
                                      .arg(server.errorString())
                                      .toUtf8();
        QSKIP(reason.constData());
    }

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
    QCOMPARE(stateSpy.last().at(0).value<IComsBackend::State>(), IComsBackend::State::Error);
}

QTEST_MAIN(BackendComsTests)

#include "backend_coms_tests.moc"
