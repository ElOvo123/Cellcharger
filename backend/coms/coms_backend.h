#ifndef COMS_BACKEND_H
#define COMS_BACKEND_H

#include <QObject>
#include <QSerialPort>
#include <QTcpSocket>
#include <QUdpSocket>

class ComsBackend : public QObject
{
    Q_OBJECT

public:
    explicit ComsBackend(QObject *parent = nullptr);

    void init_serial();
    void init_socket_vcan();
    void init_socket_udp();
    void init_socket_tcp();
    void close_serial();
    void close_socket_vcan();
    void close_socket_udp();
    void close_socket_tcp();
    void close_all();

private:
    QSerialPort *m_serial = nullptr;
    QTcpSocket  *m_tcp = nullptr;
    QUdpSocket  *m_udp = nullptr;

    int m_can_socket = -1;
};

#endif
