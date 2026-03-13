#include "coms_controller.h"
#include <iostream>

ComsController::ComsController(Coms *view, IComsBackend *backend, QObject *parent)
    : QObject(parent),
      m_view(view),
      m_backend(backend)
{
    connect(m_view, &Coms::typeChanged,
            this, &ComsController::onTypeChanged);

    connect(m_view, &Coms::connectToggled,
            this, &ComsController::onConnectToggled);

    connect(m_backend, &IComsBackend::stateChanged,
            this, &ComsController::onBackendStateChanged);

    connect(m_backend, &IComsBackend::statusMessage,
            this, &ComsController::onBackendStatusMessage);

    connect(m_backend, &IComsBackend::messageReceived,
            this, &ComsController::onBackendMessageReceived);

    onTypeChanged(0);
}

void ComsController::onTypeChanged(int)
{
    ComsType type = m_view->currentType();
    m_backend->setType(type);

    switch (type)
    {
        case ComsType::Serial:
            std::cout << "Serial selected" << std::endl;
            break;

        case ComsType::Socket_vcan:
            std::cout << "Socket vcan selected" << std::endl;
            break;

        case ComsType::Socket_UDP:
            std::cout << "Socket UDP selected" << std::endl;
            break;

        case ComsType::Socket_TCP:
            std::cout << "Socket TCP selected" << std::endl;
            break;
    }
}

void ComsController::onConnectToggled(bool connected)
{
    if (connected)
    {
        m_backend->setConfig(m_view->currentConfig());
        m_backend->connectTransport();
    }
    else
    {
        m_backend->disconnectTransport();
    }
}

void ComsController::onBackendStateChanged(IComsBackend::State state)
{
    switch (state)
    {
        case IComsBackend::State::Disconnected:
            m_view->setStatusText("Status: Disconnected");
            m_view->setConnectedUI(false);
            break;

        case IComsBackend::State::Connecting:
            m_view->setStatusText("Status: Connecting...");
            break;

        case IComsBackend::State::Connected:
            m_view->setStatusText("Status: Connected");
            m_view->setConnectedUI(true);
            break;

        case IComsBackend::State::Error:
            m_view->setStatusText("Status: Error");
            m_view->setConnectedUI(false);
            break;
    }
}

void ComsController::onBackendStatusMessage(const QString &message)
{
    std::cout << message.toStdString() << std::endl;
}

void ComsController::onBackendMessageReceived(const QString &message)
{
    std::cout << "RX: " << message.toStdString() << std::endl;
}