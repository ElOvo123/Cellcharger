#ifndef COMS_BACKEND_H
#define COMS_BACKEND_H

#include <QObject>
#include <QSerialPort>
#include <QTcpSocket>
#include <QUdpSocket>

#include "coms.h"
#include "coms_config.h"

class ComsBackend : public QObject
{
    Q_OBJECT

public:
    enum class State
    {
        Disconnected,
        Connecting,
        Connected,
        Error
    };

    explicit ComsBackend(QObject *parent = nullptr);
    ~ComsBackend();

    void setType(ComsType type);

    bool connectTransport();
    void disconnectTransport();

    State state() const;

    void setConfig(const ComsConfig &config);

signals:
    void connected();
    void disconnected();
    void errorOccurred(QString message);
    void stateChanged(State newState);

private:
    bool initSerial();
    bool initSocketCAN();
    bool initUDP();
    bool initTCP();

    void cleanup();

private:
    ComsType m_type = ComsType::Serial;
    State m_state = State::Disconnected;

    QSerialPort *m_serial = nullptr;
    QTcpSocket  *m_tcp = nullptr;
    QUdpSocket  *m_udp = nullptr;
    int m_can_socket = -1;

    ComsConfig m_config;
};

#endif
