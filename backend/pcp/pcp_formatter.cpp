#include "pcp_formatter.h"

QString PCPFormatter::toConsoleString(const PCPFrame& frame,
                                      const QString& direction,
                                      const PCPDatabase& database)
{
    const PCPIdLayout& layout = database.idLayout();
    const uint32_t messageIdMask = (1u << layout.messageIdBits) - 1u;
    const uint32_t messageId = frame.id & messageIdMask;
    const uint32_t deviceId = frame.id >> layout.messageIdBits;

    const QString deviceText =
        QString("%1 (%2)")
            .arg(QString::fromStdString(database.deviceName(deviceId)))
            .arg(deviceId);

    QString messageName = "unknown";
    const PCPMessageDefinition* msgDef = database.messageById(deviceId, messageId);
    if (msgDef)
        messageName = QString::fromStdString(msgDef->name);

    QString dataString;
    for (int i = 0; i < frame.dlc; ++i)
    {
        dataString += QString("%1 ")
                          .arg(frame.data[i], 2, 16, QChar('0'))
                          .toUpper();
    }

    return QString("%1 | %2 | %3 | Message %4 | DLC %5 | %6")
        .arg(direction)
        .arg(deviceText)
        .arg(messageName)
        .arg(messageId)
        .arg(frame.dlc)
        .arg(dataString.trimmed());
}