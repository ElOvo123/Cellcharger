#include "pcp_formatter.h"

QString PCPFormatter::toConsoleString(const PCPFrame& frame, const QString& direction)
{
    QString dataString;

    for (int i = 0; i < frame.dlc; ++i)
    {
        dataString += QString("%1 ")
                          .arg(frame.data[i], 2, 16, QChar('0'))
                          .toUpper();
    }

    return QString("%1 0x%2 [%3] %4")
        .arg(direction)
        .arg(frame.id, 3, 16, QChar('0'))
        .toUpper()
        .arg(frame.dlc)
        .arg(dataString.trimmed());
}