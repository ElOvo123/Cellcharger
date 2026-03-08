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

void Logger::log(const QString& message)
{
    const QString timestamp = QDateTime::currentDateTime().toString("[hh:mm:ss] ");
    const QString threadInfo = QString("[T%1] ").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));

    const QString fullMessage = timestamp + threadInfo + message;

    {
        QMutexLocker locker(&m_mutex);
        m_history.append(fullMessage);

        const int maxEntries = 5000;
        if (m_history.size() > maxEntries)
        {
            m_history.removeFirst();
        }
    }

    emit newLogMessage(fullMessage);
}

QStringList Logger::history() const
{
    QMutexLocker locker(&m_mutex);
    return m_history;
}