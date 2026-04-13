#include "coms_backend.h"
#include "logger_backend.h"

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

ComsBackend::ComsBackend(QObject *parent) : IComsBackend(parent)
{
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

void ComsBackend::setConfig(const ComsConfig &config)
{
    m_config = config;
}

void ComsBackend::connectTransport()
{
    if (m_state != State::Disconnected)
        return;

    setState(State::Connecting);

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
        setState(State::Connected);

        std::cout << "Connected" << std::endl;
        emit connected();
        emit statusMessage("Real backend connected");

        Logger::instance().logStatus("COMS: real backend connected");
    }
    else
    {
        cleanup();
        setState(State::Error);

        std::cout << "Connection failed" << std::endl;
        emit errorOccurred("Connection failed");
        emit statusMessage("Connection failed");

        Logger::instance().logStatus("COMS: connection failed");
    }
}

void ComsBackend::disconnectTransport()
{
    cleanup();
    setState(State::Disconnected);

    std::cout << "Disconnected" << std::endl;
    emit disconnected();
    emit statusMessage("Real backend disconnected");

    Logger::instance().logStatus("COMS: real backend disconnected");
}

bool ComsBackend::initSerial()
{
    m_serial = new QSerialPort(this);

    m_serial->setPortName(m_config.serialPort);
    m_serial->setBaudRate(m_config.baudrate);

    if (!m_serial->open(QIODevice::ReadWrite))
        return false;

    std::cout << "Serial initialized on " << m_config.serialPort.toStdString() << std::endl;

    Logger::instance().logStatus(QString("COMS: serial initialized on %1").arg(m_config.serialPort));

    return true;
}

bool ComsBackend::initSocketCAN()
{
    const QString canInterface = normalizedCanInterface(m_config.canInterface);

    m_can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (m_can_socket < 0)
    {
        std::cout << "CAN socket creation failed" << std::endl;
        Logger::instance().logStatus("COMS: CAN socket creation failed");
        return false;
    }

    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name,
                 canInterface.toLocal8Bit().constData(),
                 IFNAMSIZ - 1);

    if (ioctl(m_can_socket, SIOCGIFINDEX, &ifr) < 0)
    {
        std::cout << "CAN ioctl failed for "
                  << canInterface.toStdString() << std::endl;
        Logger::instance().logStatus(
            QString("COMS: CAN ioctl failed for %1").arg(canInterface));
        return false;
    }

    struct sockaddr_can addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(m_can_socket, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0)
    {
        std::cout << "CAN bind failed for "
                  << canInterface.toStdString() << std::endl;
        Logger::instance().logStatus(
            QString("COMS: CAN bind failed for %1").arg(canInterface));
        return false;
    }

    std::cout << "SocketCAN initialized on "
              << canInterface.toStdString() << std::endl;
    Logger::instance().logStatus(
        QString("COMS: SocketCAN initialized on %1").arg(canInterface));

    return true;
}

bool ComsBackend::initUDP()
{
    m_udp = new QUdpSocket(this);

    if (!m_udp->bind(QHostAddress::Any, 5000))
    {
        std::cout << "UDP bind failed" << std::endl;
        Logger::instance().logStatus("COMS: UDP bind failed on port 5000");
        return false;
    }

    std::cout << "UDP initialized on port 5000" << std::endl;
    Logger::instance().logStatus("COMS: UDP initialized on port 5000");

    return true;
}

bool ComsBackend::initTCP()
{
    m_tcp = new QTcpSocket(this);

    m_tcp->connectToHost(m_config.ip, m_config.port);

    if (!m_tcp->waitForConnected(3000))
    {
        Logger::instance().logStatus( QString("COMS: TCP connection failed to %1:%2") .arg(m_config.ip) .arg(m_config.port));
        return false;
    }

    std::cout << "TCP connected to " << m_config.ip.toStdString() << ":" << m_config.port << std::endl;
    Logger::instance().logStatus(QString("COMS: TCP connected to %1:%2") .arg(m_config.ip) .arg(m_config.port));

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

void ComsBackend::setState(State state)
{
    if (m_state == state)
        return;

    m_state = state;
    emit stateChanged(m_state);
}
