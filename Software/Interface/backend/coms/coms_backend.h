#pragma once

#include "icomms_backend.h"
#include "../pcp/pcp_encoder.h"

#include <QSerialPort>
#include <QTcpSocket>
#include <QUdpSocket>

class ComsBackend : public IComsBackend
{
    Q_OBJECT

public:
    explicit ComsBackend(QObject* parent = nullptr);
    ~ComsBackend() override;

    void setType(ComsType type) override;
    void setConfig(const ComsConfig& config) override;
    void connectTransport() override;
    void disconnectTransport() override;
    bool sendFrame(const PCPFrame& frame, const PCPDatabase& database) override;

private:
    bool initSerial();
    bool initSocketCAN();
    bool initUDP();
    bool initTCP();
    void handleSerialReadyRead();
    void handleTcpReadyRead();
    void handleUdpReadyRead();
    void handleTcpDisconnected();
    void publishReceivedPayload(const QByteArray& payload);
    void handleReceiveFailure(const QString& reason);
    void cleanup();
    void setState(State state);

private:
    ComsType m_type = ComsType::Serial;
    ComsConfig m_config;
    State m_state = State::Disconnected;

    QSerialPort* m_serial = nullptr;
    QTcpSocket* m_tcp = nullptr;
    QUdpSocket* m_udp = nullptr;
    int m_can_socket = -1;
};
