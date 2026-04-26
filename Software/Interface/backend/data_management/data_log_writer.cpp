#include "data_log_writer.h"

namespace
{
QString timestampedPath(const QDir& directory, const QString& stem)
{
    const QString timestamp = QDateTime::currentDateTimeUtc().toString("yyyyMMdd_HHmmss_zzz");
    return directory.filePath(QString("%1_%2.csv").arg(stem, timestamp));
}

bool ensureDirectory(const QDir& directory, QString* errorMessage)
{
    if (directory.exists())
        return true;

    if (QDir().mkpath(directory.absolutePath()))
        return true;

    if (errorMessage)
        *errorMessage = QString("could not create directory %1").arg(directory.absolutePath());
    return false;
}
} // namespace

bool CsvLogWriter::open(const QDir& directory, const QString& stem, QString* errorMessage)
{
    if (!ensureDirectory(directory, errorMessage))
        return false;

    m_file.setFileName(timestampedPath(directory, stem));
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (errorMessage)
            *errorMessage = m_file.errorString();
        return false;
    }

    QTextStream out(&m_file);
    out << "timestamp_ms,cycle_index,step_index,step_name,voltage_v,current_a,temperature_deg_c,pressure_bar,"
           "capacity_ah,energy_wh\n";
    return true;
}

bool CsvLogWriter::append(const LogRecord& record, QString* errorMessage)
{
    if (!m_file.isOpen())
    {
        if (errorMessage)
            *errorMessage = "log file is not open";
        return false;
    }

    QTextStream out(&m_file);
    out << record.timestampMs << ',' << record.cycleIndex << ',' << record.stepIndex << ',' << record.stepName << ','
        << QString::number(record.sample.voltageV, 'f', 6) << ','
        << QString::number(record.sample.currentA, 'f', 6) << ','
        << QString::number(record.sample.temperatureDegC, 'f', 3) << ','
        << QString::number(record.sample.pressureBar, 'f', 6) << ',' << QString::number(record.capacityAh, 'f', 9)
        << ',' << QString::number(record.energyWh, 'f', 9) << '\n';
    return true;
}

bool CsvLogWriter::flush(QString* errorMessage)
{
    if (!m_file.isOpen())
    {
        if (errorMessage)
            *errorMessage = "log file is not open";
        return false;
    }

    m_file.flush();
    return true;
}

QString CsvLogWriter::filePath() const
{
    return m_file.fileName();
}

bool AlarmLogWriter::open(const QDir& directory, const QString& stem, QString* errorMessage)
{
    if (!ensureDirectory(directory, errorMessage))
        return false;

    m_file.setFileName(timestampedPath(directory, stem));
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (errorMessage)
            *errorMessage = m_file.errorString();
        return false;
    }

    QTextStream out(&m_file);
    out << "timestamp_ms,code,cause\n";
    return true;
}

bool AlarmLogWriter::append(const AlarmRecord& record, QString* errorMessage)
{
    if (!m_file.isOpen())
    {
        if (errorMessage)
            *errorMessage = "alarm log file is not open";
        return false;
    }

    QTextStream out(&m_file);
    out << record.timestampMs << ',' << record.code << ',' << record.cause << '\n';
    m_file.flush();
    return true;
}

QString AlarmLogWriter::filePath() const
{
    return m_file.fileName();
}
