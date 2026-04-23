#pragma once

#include <QMap>
#include <QString>

#include <cstdint>

struct ChargerStatusCommand
{
    uint32_t chargerId = 0;
    int mode = 0;
    bool start = false;
    double setpoint = 0.0;
};

struct ParsedChargerStatusMessage
{
    bool valid = false;
    uint32_t chargerId = 0;
    QMap<QString, QString> signalValues;
    QString voltageText;
    QString currentText;
    QString tempText;
    QString statusText;
};

class ChargerStatusLogic
{
public:
    static bool parseNumericSignal(const QMap<QString, QString>& signalValues,
                                   const QString& signalName,
                                   double& value);
    static QString signalDisplay(const QMap<QString, QString>& signalValues,
                                 const QString& primaryName,
                                 const QString& fallbackName = QString(),
                                 const QString& secondFallbackName = QString());
    static QString overviewMarkup(const QString& voltText,
                                  const QString& currentText,
                                  const QString& tempText,
                                  const QString& statusText);
    static QString formattedStatusText(const QMap<QString, QString>& signalValues);
    static ParsedChargerStatusMessage parseDecodedStatusMessage(const QString& message);
    static ChargerStatusCommand generalCommandForSlot(int slotIndex,
                                                      bool assigned,
                                                      uint32_t deviceId,
                                                      int mode,
                                                      bool start);
    static ChargerStatusCommand detailedCommand(uint32_t chargerId,
                                                bool cvMode,
                                                double setpoint,
                                                bool start);
};
