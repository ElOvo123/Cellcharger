#include "coms_controller.h"
#include <iostream>

ComsController::ComsController(Coms *view, ComsBackend *backend, QObject *parent) : QObject(parent), m_view(view), m_backend(backend)
{
    connect(m_view, &Coms::typeChanged, this, &ComsController::onTypeChanged);
    connect(m_view, &Coms::connectToggled, this, &ComsController::onConnectToggled);
    connect(m_backend, &ComsBackend::stateChanged, this, &ComsController::onBackendStateChanged);

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

void ComsController::onBackendStateChanged(ComsBackend::State state)
{
    switch (state)
    {
        case ComsBackend::State::Disconnected:
            m_view->setStatusText("Status: Disconnected");
            m_view->setConnectedUI(false);
            break;

        case ComsBackend::State::Connecting:
            m_view->setStatusText("Status: Connecting...");
            break;

        case ComsBackend::State::Connected:
            m_view->setStatusText("Status: Connected");
            m_view->setConnectedUI(true);
            break;

        case ComsBackend::State::Error:
            m_view->setStatusText("Status: Error");
            m_view->setConnectedUI(false);
            break;
    }
}
