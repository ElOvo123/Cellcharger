#include "charger_status_logic.h"

#include <QRegularExpression>

bool ChargerStatusLogic::parseNumericSignal(const QMap<QString, QString>& signalValues,
                                            const QString& signalName,
                                            double& value)
{
    if (!signalValues.contains(signalName))
        return false;

    bool ok = false;
    value = signalValues.value(signalName).toDouble(&ok);
    return ok;
}

QString ChargerStatusLogic::signalDisplay(const QMap<QString, QString>& signalValues,
                                          const QString& primaryName,
                                          const QString& fallbackName,
                                          const QString& secondFallbackName)
{
    if (signalValues.contains(primaryName))
        return signalValues.value(primaryName);

    if (!fallbackName.isEmpty() && signalValues.contains(fallbackName))
        return signalValues.value(fallbackName);

    if (!secondFallbackName.isEmpty() && signalValues.contains(secondFallbackName))
        return signalValues.value(secondFallbackName);

    return "--";
}

QString ChargerStatusLogic::overviewMarkup(const QString& voltText,
                                           const QString& currentText,
                                           const QString& tempText,
                                           const QString& statusText)
{
    return QString(
               "<span style='color:#486581;'>Voltage</span><br><b>%1 V</b><br>"
               "<span style='color:#486581;'>Current</span><br><b>%2 A</b><br>"
               "<span style='color:#486581;'>Temp</span><br><b>%3 C</b><br>"
               "<span style='color:#486581;'>Status</span><br><b>%4</b>")
        .arg(voltText)
        .arg(currentText)
        .arg(tempText)
        .arg(statusText);
}

QString ChargerStatusLogic::formattedStatusText(const QMap<QString, QString>& signalValues)
{
    QString statusValue;
    if (signalValues.contains("status"))
        statusValue = signalValues.value("status");
    else if (signalValues.contains("state"))
        statusValue = signalValues.value("state");
    else
        return "--";

    bool ok = false;
    const int statusCode = statusValue.toInt(&ok);
    if (!ok)
        return statusValue;

    switch (statusCode)
    {
        case 0:
            return "Idle";
        case 1:
            return "Charging";
        case 2:
        {
            bool faultOk = false;
            const int faultCode = signalValues.value("fault_code", "0").toInt(&faultOk);
            return faultOk ? QString("Fault %1").arg(faultCode) : QString("Fault");
        }
        default:
            return QString::number(statusCode);
    }
}

ParsedChargerStatusMessage ChargerStatusLogic::parseDecodedStatusMessage(const QString& message)
{
    ParsedChargerStatusMessage parsed;
    const QStringList lines = message.split('\n', Qt::SkipEmptyParts);
    if (lines.isEmpty())
        return parsed;

    static const QRegularExpression headerRegex(
        "PCP\\s+([A-Za-z_][A-Za-z0-9_]*)\\s+\\|\\s+DEV=(\\d+)");
    const QRegularExpressionMatch headerMatch = headerRegex.match(lines.first());
    if (!headerMatch.hasMatch())
        return parsed;

    const QString messageName = headerMatch.captured(1).toLower();
    if (messageName != "status")
        return parsed;

    bool ok = false;
    const uint32_t headerDeviceId = headerMatch.captured(2).toUInt(&ok);
    if (!ok)
        return parsed;

    static const QRegularExpression signalRegex("^\\s*([A-Za-z0-9_]+)\\s*=\\s*([^\\s]+)");
    for (const QString& line : lines)
    {
        const QRegularExpressionMatch match = signalRegex.match(line);
        if (!match.hasMatch())
            continue;

        parsed.signalValues.insert(match.captured(1).toLower(), match.captured(2));
    }

    uint32_t chargerId = headerDeviceId;
    if (parsed.signalValues.contains("charger_id"))
        chargerId = parsed.signalValues.value("charger_id").toUInt(&ok);
    else
        ok = true;

    if (!ok)
        return ParsedChargerStatusMessage{};

    parsed.valid = true;
    parsed.chargerId = chargerId;
    parsed.voltageText = signalDisplay(parsed.signalValues, "volt", "voltage");
    parsed.currentText = signalDisplay(parsed.signalValues, "current");
    parsed.tempText = signalDisplay(parsed.signalValues, "temp", "temperature");
    parsed.statusText = formattedStatusText(parsed.signalValues);
    return parsed;
}

ChargerStatusCommand ChargerStatusLogic::generalCommandForSlot(int slotIndex,
                                                               bool assigned,
                                                               uint32_t deviceId,
                                                               int mode,
                                                               bool start)
{
    ChargerStatusCommand command;
    command.chargerId = assigned ? deviceId : static_cast<uint32_t>(slotIndex + 1);
    command.mode = mode;
    command.start = start;
    command.setpoint = start ? 4.2 : 0.0;
    return command;
}

ChargerStatusCommand ChargerStatusLogic::detailedCommand(uint32_t chargerId,
                                                         bool cvMode,
                                                         double setpoint,
                                                         bool start)
{
    ChargerStatusCommand command;
    command.chargerId = chargerId;
    command.mode = cvMode ? 1 : 0;
    command.start = start;
    command.setpoint = setpoint;
    return command;
}
