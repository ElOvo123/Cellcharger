#include "pcp_decode_formatter.h"

QString PCPDecodeFormatter::toText(const PCPDecodedMessage& msg)
{
    QString out;

    out += QString("PCP %1 | DEV=%2 | NAME=%3 | MSG=%4\n")
               .arg(QString::fromStdString(msg.messageName))
               .arg(msg.deviceId)
               .arg(QString::fromStdString(msg.deviceName))
               .arg(msg.messageId);

    for (const auto& [name, sig] : msg.decodedSignals)
    {
        out += QString("  %1 = %2 (raw=%3)\n")
                   .arg(QString::fromStdString(name))
                   .arg(sig.physicalValue)
                   .arg(sig.rawValue);
    }

    return out.trimmed();
}