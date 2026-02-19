#include "coms_controller.h"
#include <iostream>

ComsController::ComsController(Coms *view, QObject *parent) : QObject(parent), m_view(view)
{
    connect(m_view,&Coms::typeChanged, this, &ComsController::onTypeChanged);
    onTypeChanged(0);
}

void ComsController::onTypeChanged(int)
{
    ComsType type = m_view->currentType();

    switch (type)
    {
        case ComsType::Serial:
            std::cout << "Controller: Serial selected" << std::endl;
            backend.close_all();
            backend.init_serial();
            break;

        case ComsType::Socket_vcan:
            std::cout << "Controller: Socket vcan selected" << std::endl;
            backend.close_all();
            backend.init_socket_vcan();
            break;

        case ComsType::Socket_UDP:
            std::cout << "Controller: Socket UDP selected" << std::endl;
            backend.close_all();
            backend.init_socket_udp();
            break;

        case ComsType::Socket_TCP:
            std::cout << "Controller: Socket TCP selected" << std::endl;
            backend.close_all();
            backend.init_socket_tcp();
            break;
    }

    // Here you would:
    // - Enable/disable fields
    // - Initialize serial
    // - Setup socket config
    // - Update model
}
