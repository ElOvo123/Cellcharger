#pragma once

#include <QObject>
#include "coms_types.h"

class IComsBackend : public QObject
{
    Q_OBJECT

public:
    explicit IComsBackend(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    virtual ~IComsBackend() = default;

    enum class State
    {
        Disconnected,
        Connecting,
        Connected,
        Error
    };
    Q_ENUM(State)

    virtual void setType(ComsType type) = 0;
    virtual void setConfig(const ComsConfig &config) = 0;
    virtual void connectTransport() = 0;
    virtual void disconnectTransport() = 0;

signals:
    void stateChanged(IComsBackend::State state);
    void connected();
    void disconnected();
    void errorOccurred(const QString &message);
    void messageReceived(const QString &message);
    void messageSent(const QString &message);
    void statusMessage(const QString &message);
};