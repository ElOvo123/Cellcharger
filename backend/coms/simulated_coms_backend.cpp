#include "simulated_coms_backend.h"
#include "logger_backend.h"

SimulatedComsBackend::SimulatedComsBackend(QObject *parent)
    : IComsBackend(parent)
{
    connect(&m_timer,
            &QTimer::timeout,
            this,
            &SimulatedComsBackend::generateFakeMessage);
}

void SimulatedComsBackend::setType(ComsType type)
{
    m_type = type;
}

void SimulatedComsBackend::setConfig(const ComsConfig &config)
{
    m_config = config;
}

void SimulatedComsBackend::connectTransport()
{
    setState(State::Connecting);

    m_counter = 0;
    m_timer.start(1000);

    setState(State::Connected);

    emit statusMessage("Simulated backend connected");
    Logger::instance().log("COMS: simulated backend connected");
}

void SimulatedComsBackend::disconnectTransport()
{
    m_timer.stop();

    setState(State::Disconnected);

    emit statusMessage("Simulated backend disconnected");
    Logger::instance().log("COMS: simulated backend disconnected");
}

void SimulatedComsBackend::generateFakeMessage()
{
    ++m_counter;

    const QString msg =
        QString("SIM RX #%1 | V=3.%2 I=0.%3 T=2%4")
            .arg(m_counter)
            .arg((m_counter % 40) + 60)
            .arg((m_counter % 7) + 2)
            .arg(m_counter % 10);

    emit messageReceived(msg);
    Logger::instance().log(msg);
}

void SimulatedComsBackend::setState(State state)
{
    if (m_state == state)
        return;

    m_state = state;
    emit stateChanged(m_state);
}