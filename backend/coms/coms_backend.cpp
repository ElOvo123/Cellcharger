#include "coms_backend.h"

#include <QDebug>
#include <iostream>
#include <QSerialPortInfo>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>

ComsBackend::ComsBackend(QObject *parent) : QObject(parent)
{
}

void ComsBackend::init_serial()
{
    if (m_serial)
        m_serial->deleteLater();

    m_serial = new QSerialPort(this);

    m_serial->setPortName("/dev/ttyUSB0");
    m_serial->setBaudRate(QSerialPort::Baud115200);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (m_serial->open(QIODevice::ReadWrite))
        std::cout << "Serial initialized" << std::endl;
    else
        std::cout << "Serial failed:" << m_serial->errorString().toStdString() << std::endl;
}

void ComsBackend::init_socket_vcan()
{
    m_can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);

    if (m_can_socket < 0)
    {
        std::cout << "CAN socket creation failed" << std::endl;
        return;
    }

    struct ifreq ifr;
    strcpy(ifr.ifr_name, "vcan0");
    ioctl(m_can_socket, SIOCGIFINDEX, &ifr);

    struct sockaddr_can addr;
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(m_can_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        qDebug() << "CAN bind failed";
        close(m_can_socket);
        return;
    }

    std::cout << "SocketCAN (vcan0) initialized" << std::endl;
}

void ComsBackend::init_socket_udp()
{
    if (m_udp)
        m_udp->deleteLater();

    m_udp = new QUdpSocket(this);

    if (m_udp->bind(QHostAddress::Any, 5000))
        std::cout << "UDP socket initialized on port 5000" << std::endl;
    else
        std::cout << "UDP bind failed" << std::endl;
}

void ComsBackend::init_socket_tcp()
{
    if (m_tcp)
        m_tcp->deleteLater();

    m_tcp = new QTcpSocket(this);

    m_tcp->connectToHost("127.0.0.1", 6000);

    connect(m_tcp, &QTcpSocket::connected, this, [](){ std::cout << "TCP connected" << std::endl; });
    connect(m_tcp, &QTcpSocket::errorOccurred, this, [](QAbstractSocket::SocketError error) { std::cout << "TCP connection error: " << error << std::endl; });
}

void ComsBackend::close_serial()
{
    if (m_serial)
    {
        if (m_serial->isOpen())
        {
            m_serial->close();
            std::cout << "Serial closed" << std::endl;
        }

        m_serial->deleteLater();
        m_serial = nullptr;
    }
}

void ComsBackend::close_socket_vcan()
{
    if (m_can_socket >= 0)
    {
        close(m_can_socket);
        m_can_socket = -1;
        std::cout << "CAN socket closed" << std::endl;
    }
}

void ComsBackend::close_socket_udp()
{
    if (m_udp)
    {
        m_udp->close();
        std::cout << "UDP socket closed" << std::endl;

        m_udp->deleteLater();
        m_udp = nullptr;
    }
}

void ComsBackend::close_socket_tcp()
{
    if (m_tcp)
    {
        if (m_tcp->isOpen())
        {
            m_tcp->disconnectFromHost();
            m_tcp->close();
            std::cout << "TCP socket closed" << std::endl;
        }

        m_tcp->deleteLater();
        m_tcp = nullptr;
    }
}

void ComsBackend::close_all()
{
    close_serial();
    close_socket_vcan();
    close_socket_udp();
    close_socket_tcp();
}

