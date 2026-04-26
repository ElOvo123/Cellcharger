#pragma once

#include "measurement_engine.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QString>
#include <QTextStream>

struct LogRecord
{
    qint64 timestampMs = 0;
    int cycleIndex = 0;
    int stepIndex = 0;
    QString stepName;
    MeasurementSample sample;
    double capacityAh = 0.0;
    double energyWh = 0.0;
};

struct AlarmRecord
{
    qint64 timestampMs = 0;
    QString code;
    QString cause;
};

class CsvLogWriter
{
public:
    bool open(const QDir& directory, const QString& stem, QString* errorMessage = nullptr);
    bool append(const LogRecord& record, QString* errorMessage = nullptr);
    bool flush(QString* errorMessage = nullptr);
    QString filePath() const;

private:
    QFile m_file;
};

class AlarmLogWriter
{
public:
    bool open(const QDir& directory, const QString& stem, QString* errorMessage = nullptr);
    bool append(const AlarmRecord& record, QString* errorMessage = nullptr);
    QString filePath() const;

private:
    QFile m_file;
};
