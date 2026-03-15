#include "simulated_coms_backend.h"
#include "logger_backend.h"
#include "pcp_formatter.h"

SimulatedComsBackend::SimulatedComsBackend(QObject *parent) : IComsBackend(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &SimulatedComsBackend::generateFakeMessage);
    m_pcpEncoder.loadFromFile("pcp.yaml");
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
    Logger::instance().logStatus("COMS: simulated backend connected");
}

void SimulatedComsBackend::disconnectTransport()
{
    m_timer.stop();

    setState(State::Disconnected);

    emit statusMessage("Simulated backend disconnected");
    Logger::instance().logStatus("COMS: simulated backend disconnected");
}

void SimulatedComsBackend::generateFakeMessage()
{
    ++m_counter;

    const double voltage = 12.0 + (m_counter % 30) * 0.05;
    const double current = -2.0 + (m_counter % 20) * 0.2;
    const double temperature = 25.0 + (m_counter % 10);
    const double state = m_counter % 5;

    PCPFrame frame = m_pcpEncoder.encode(
        "status",
        m_deviceId,
        {
            {"voltage", voltage},
            {"current", current},
            {"temperature", temperature},
            {"state", state}
        });

    const QString msg = PCPFormatter::toConsoleString(frame, "RX");

    emit messageReceived(msg);
    Logger::instance().logComs(msg);
}

void SimulatedComsBackend::setState(State state)
{
    if (m_state == state)
        return;

    m_state = state;
    emit stateChanged(m_state);
}