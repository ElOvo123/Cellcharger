#include "logger_backend.h"

#include <QDateTime>
#include <QMutexLocker>
#include <QThread>

Logger::Logger(QObject *parent) : QObject(parent)
{
}

Logger& Logger::instance()
{
    static Logger instance;
    return instance;
}

QString Logger::formatMessage(const QString& message) const
{
    const QString timestamp =
        QDateTime::currentDateTime().toString("[hh:mm:ss] ");

    const QString threadInfo =
        QString("[T%1] ").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));

    return timestamp + threadInfo + message;
}

void Logger::logStatus(const QString& message)
{
    const QString fullMessage = formatMessage(message);

    {
        QMutexLocker locker(&m_mutex);
        m_statusHistory.append(fullMessage);
        if (m_statusHistory.size() > 5000)
            m_statusHistory.removeFirst();
    }

    emit newStatusMessage(fullMessage);
}

void Logger::logComs(const QString& message)
{
    const QString fullMessage = formatMessage(message);

    {
        QMutexLocker locker(&m_mutex);
        m_comsHistory.append(fullMessage);
        if (m_comsHistory.size() > 5000)
            m_comsHistory.removeFirst();
    }

    emit newComsMessage(fullMessage);
}

void Logger::logDecoded(const QString& message)
{
    const QString fullMessage = formatMessage(message);

    {
        QMutexLocker locker(&m_mutex);
        m_decodedHistory.append(fullMessage);
        if (m_decodedHistory.size() > 5000)
            m_decodedHistory.removeFirst();
    }

    emit newDecodedMessage(fullMessage);
}

QStringList Logger::statusHistory() const
{
    QMutexLocker locker(&m_mutex);
    return m_statusHistory;
}

QStringList Logger::comsHistory() const
{
    QMutexLocker locker(&m_mutex);
    return m_comsHistory;
}

QStringList Logger::decodedHistory() const
{
    QMutexLocker locker(&m_mutex);
    return m_decodedHistory;
}