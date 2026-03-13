#pragma once

#include <QString>

enum class ComsType
{
    Serial,
    Socket_vcan,
    Socket_UDP,
    Socket_TCP
};

struct ComsConfig
{
    QString serialPort;
    int baudrate = 115200;

    QString ip;
    int port = 0;
};