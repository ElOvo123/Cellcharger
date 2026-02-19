#include "coms_backend.h"

#include <iostream>
#include <QHostAddress>
#include <QSerialPortInfo>

#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>

ComsBackend::ComsBackend(QObject *parent) : QObject(parent){
}

ComsBackend::~ComsBackend()
{
    cleanup();
}

void ComsBackend::setType(ComsType type)
{
    if (m_state == State::Connected)
        disconnectTransport();

    m_type = type;
}

ComsBackend::State ComsBackend::state() const
{
    return m_state;
}

bool ComsBackend::connectTransport()
{
    if (m_state != State::Disconnected)
        return false;

    m_state = State::Connecting;
    emit stateChanged(m_state);

    bool success = false;

    switch (m_type)
    {
        case ComsType::Serial:
            success = initSerial();
            break;

        case ComsType::Socket_vcan:
            success = initSocketCAN();
            break;

        case ComsType::Socket_UDP:
            success = initUDP();
            break;

        case ComsType::Socket_TCP:
            success = initTCP();
            break;
    }

    if (success)
    {
        m_state = State::Connected;
        std::cout << "Connected" << std::endl;
        emit connected();
    }
    else{
        cleanup();
        m_state = State::Error;
        std::cout << "Connection failed" << std::endl;
        emit errorOccurred("Connection failed");
    }

    emit stateChanged(m_state);
    return success;
}

void ComsBackend::disconnectTransport()
{
    cleanup();
    m_state = State::Disconnected;
    std::cout << "Disconnected" << std::endl;
    emit disconnected();
    emit stateChanged(m_state);
}

bool ComsBackend::initSerial()
{
    m_serial = new QSerialPort(this);

    m_serial->setPortName(m_config.serialPort);
    m_serial->setBaudRate(m_config.baudrate);

    if (!m_serial->open(QIODevice::ReadWrite))
        return false;

    std::cout << "Serial initialized on " << m_config.serialPort.toStdString() << std::endl;

    return true;
}

bool ComsBackend::initSocketCAN()
{
    m_can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (m_can_socket < 0)
    {
        std::cout << "CAN socket creation failed" << std::endl;
        return false;
    }

    struct ifreq ifr;
    std::strcpy(ifr.ifr_name, "vcan0");

    if (ioctl(m_can_socket, SIOCGIFINDEX, &ifr) < 0)
    {
        std::cout << "CAN ioctl failed" << std::endl;
        return false;
    }

    struct sockaddr_can addr;
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(m_can_socket, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0)
    {
        std::cout << "CAN bind failed" << std::endl;
        return false;
    }

    std::cout << "SocketCAN initialized" << std::endl;
    return true;
}

bool ComsBackend::initUDP()
{
    m_udp = new QUdpSocket(this);

    if (!m_udp->bind(QHostAddress::Any, 5000))
    {
        std::cout << "UDP bind failed" << std::endl;
        return false;
    }

    std::cout << "UDP initialized on port 5000" << std::endl;
    return true;
}

bool ComsBackend::initTCP()
{
    m_tcp = new QTcpSocket(this);

    m_tcp->connectToHost(m_config.ip, m_config.port);

    if (!m_tcp->waitForConnected(3000))
        return false;

    std::cout << "TCP connected to " << m_config.ip.toStdString() << ":" << m_config.port << std::endl;

    return true;
}

void ComsBackend::cleanup()
{
    if (m_serial)
    {
        if (m_serial->isOpen())
            m_serial->close();

        delete m_serial;
        m_serial = nullptr;
    }

    if (m_tcp)
    {
        if (m_tcp->isOpen())
            m_tcp->disconnectFromHost();

        delete m_tcp;
        m_tcp = nullptr;
    }

    if (m_udp)
    {
        m_udp->close();
        delete m_udp;
        m_udp = nullptr;
    }

    if (m_can_socket >= 0)
    {
        close(m_can_socket);
        m_can_socket = -1;
    }
}

void ComsBackend::setConfig(const ComsConfig &config)
{
    m_config = config;
}
