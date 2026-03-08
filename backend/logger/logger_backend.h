#pragma once

#include <QObject>
#include <QStringList>
#include <QMutex>

class Logger : public QObject
{
    Q_OBJECT

public:
    static Logger& instance();

    void log(const QString& message);
    QStringList history() const;

signals:
    void newLogMessage(const QString& message);

private:
    explicit Logger(QObject *parent = nullptr);

    mutable QMutex m_mutex;
    QStringList m_history;
};