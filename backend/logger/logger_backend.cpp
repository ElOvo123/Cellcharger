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
    const QString timestamp = QDateTime::currentDateTime().toString("[hh:mm:ss] ");
    const QString threadInfo = QString("[T%1] ").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));

    return timestamp + threadInfo + message;
}

void Logger::logStatus(const QString& message)
{
    const QString fullMessage = formatMessage(message);

    {
        QMutexLocker locker(&m_mutex);
        m_statusHistory.append(fullMessage);

        const int maxEntries = 5000;
        if (m_statusHistory.size() > maxEntries)
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

        const int maxEntries = 5000;
        if (m_comsHistory.size() > maxEntries)
            m_comsHistory.removeFirst();
    }

    emit newComsMessage(fullMessage);
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