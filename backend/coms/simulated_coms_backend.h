#pragma once

#include "icomms_backend.h"
#include <QTimer>

class SimulatedComsBackend : public IComsBackend
{
    Q_OBJECT

public:
    explicit SimulatedComsBackend(QObject *parent = nullptr);

    void setType(ComsType type) override;
    void setConfig(const ComsConfig &config) override;
    void connectTransport() override;
    void disconnectTransport() override;

private slots:
    void generateFakeMessage();

private:
    void setState(State state);

private:
    ComsType m_type = ComsType::Serial;
    ComsConfig m_config;
    State m_state = State::Disconnected;
    QTimer m_timer;
    int m_counter = 0;
};