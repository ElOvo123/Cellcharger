#ifndef COMS_CONFIG_H
#define COMS_CONFIG_H

#include <QString>

struct ComsConfig
{
    QString serialPort;
    int baudrate = 115200;

    QString ip;
    int port = 0;
};

#endif
