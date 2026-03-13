#pragma once

#include <QObject>
#include <QStringList>
#include <QMutex>

class Logger : public QObject
{
    Q_OBJECT

public:
    static Logger& instance();

    void logStatus(const QString& message);
    void logComs(const QString& message);

    QStringList statusHistory() const;
    QStringList comsHistory() const;

signals:
    void newStatusMessage(const QString& message);
    void newComsMessage(const QString& message);

private:
    explicit Logger(QObject *parent = nullptr);

    QString formatMessage(const QString& message) const;

private:
    mutable QMutex m_mutex;
    QStringList m_statusHistory;
    QStringList m_comsHistory;
};