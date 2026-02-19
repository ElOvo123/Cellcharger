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
            break;

        case ComsType::Socket:
            std::cout << "Controller: Socket selected" << std::endl;
            break;
    }

    // Here you would:
    // - Enable/disable fields
    // - Initialize serial
    // - Setup socket config
    // - Update model
}
