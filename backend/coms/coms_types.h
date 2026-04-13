#pragma once

#include <QString>
#include <QStringList>

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

    QString canInterface = "vcan0";
    QString ip;
    int port = 0;
};

inline QStringList availableCanInterfaces()
{
    return {"vcan0", "vcan1", "vcan2", "can0", "can1", "can2"};
}

inline QString normalizedCanInterface(const QString& candidate)
{
    const QStringList interfaces = availableCanInterfaces();
    return interfaces.contains(candidate) ? candidate : interfaces.first();
}

inline int configPageIndexForType(ComsType type)
{
    switch (type)
    {
        case ComsType::Serial:
            return 0;
        case ComsType::Socket_vcan:
            return 1;
        case ComsType::Socket_UDP:
        case ComsType::Socket_TCP:
            return 2;
    }

    return 0;
}
