#include "logger_backend.h"
#include <QDateTime>
#include <QThread>

Logger::Logger(QObject *parent)
    : QObject(parent)
{
}

Logger& Logger::instance()
{
    static Logger instance;
    return instance;
}

void Logger::log(const QString& message)
{
    QString timestamp =
        QDateTime::currentDateTime().toString("[hh:mm:ss] ");

    QString threadInfo =
        QString("[T%1] ").arg((quintptr)QThread::currentThreadId());

    QString fullMessage = timestamp + threadInfo + message;

    {
        QMutexLocker locker(&m_mutex);
        m_history.append(fullMessage);
    }

    // Thread-safe signal emission
    emit newLogMessage(fullMessage);
}

QStringList Logger::history() const
{
    QMutexLocker locker(&m_mutex);
    return m_history;
}