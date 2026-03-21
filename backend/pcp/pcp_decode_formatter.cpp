#include "pcp_decode_formatter.h"

QString PCPDecodeFormatter::toText(const PCPDecodedMessage& msg)
{
    QString out;

    out += QString("PCP %1 | CAN=0x%2 | DEV=%3 | MSG=0x%4\n") .arg(QString::fromStdString(msg.messageName)) .arg(msg.canId, 3, 16, QChar('0')) .toUpper() .arg(msg.deviceId) .arg(msg.messageId, 2, 16, QChar('0')) .toUpper();

    for (const auto& [name, sig] : msg.decodedSignals)
    {
        out += QString("  %1 = %2 (raw=%3)\n") .arg(QString::fromStdString(name)) .arg(sig.physicalValue) .arg(sig.rawValue);
    }

    return out.trimmed();
}