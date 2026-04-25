#include "simulated_coms_backend.h"
#include "../logger/logger_backend.h"
#include "../pcp/pcp_formatter.h"
#include "../pcp/pcp_decode_formatter.h"

#include <iostream>

SimulatedComsBackend::SimulatedComsBackend(const PCPDatabase* db, QObject* parent)
    : IComsBackend(parent), m_pcpDatabase(db), m_pcpEncoder(db), m_pcpDecoder(db)
{
    connect(&m_timer, &QTimer::timeout, this, &SimulatedComsBackend::generateFakeMessage);

    if (m_pcpDatabase)
        m_deviceIds = m_pcpDatabase->deviceIds();
}

void SimulatedComsBackend::setType(ComsType type)
{
    if (m_state == State::Connected)
        disconnectTransport();

    m_type = type;
}

void SimulatedComsBackend::setConfig(const ComsConfig& config)
{
    m_config = config;
}

void SimulatedComsBackend::connectTransport()
{
    if (m_state != State::Disconnected)
        return;

    if (!m_pcpDatabase)
    {
        setState(State::Error);
        emit errorOccurred("No PCP database configured");
        Logger::instance().logStatus("COMS: no PCP database configured");
        return;
    }

    m_deviceIds = m_pcpDatabase->deviceIds();
    m_deviceIndex = 0;
    m_counter = 0;
    m_messageIndexByDevice.clear();

    if (m_deviceIds.empty())
    {
        setState(State::Error);
        emit errorOccurred("No devices found in PCP database");
        Logger::instance().logStatus("COMS: no devices found in PCP database");
        return;
    }

    for (uint32_t deviceId : m_deviceIds)
        m_messageIndexByDevice[deviceId] = 0;

    setState(State::Connected);
    emit connected();
    emit statusMessage("Simulated backend connected");
    Logger::instance().logStatus("COMS: simulated backend connected");

    m_timer.start(33);
}

void SimulatedComsBackend::disconnectTransport()
{
    m_timer.stop();

    setState(State::Disconnected);
    emit disconnected();
    emit statusMessage("Simulated backend disconnected");
    Logger::instance().logStatus("COMS: simulated backend disconnected");
}

bool SimulatedComsBackend::sendFrame(const PCPFrame& frame, const PCPDatabase& database)
{
    const QString message = PCPFormatter::toConsoleString(frame, "TX", database);
    std::cout << message.toStdString() << std::endl;
    publishFrame(frame, "TX", true);
    return true;
}

void SimulatedComsBackend::setState(State state)
{
    if (m_state == state)
        return;

    m_state = state;
    emit stateChanged(m_state);
}

void SimulatedComsBackend::publishFrame(const PCPFrame& frame, const QString& direction, bool decodeFrame)
{
    const QString message = PCPFormatter::toConsoleString(frame, direction, *m_pcpDatabase);

    if (direction == "TX")
        emit messageSent(message);
    else
        emit messageReceived(message);

    if (decodeFrame)
    {
        const auto decoded = m_pcpDecoder.decode(frame.id, frame.dlc, frame.data);
        if (decoded.has_value())
            Logger::instance().logDecoded(PCPDecodeFormatter::toText(decoded.value()));
    }

    Logger::instance().logComs(message);
}

double SimulatedComsBackend::fakeValueForSignal(const std::string& signalName, uint32_t deviceId, int counter) const
{
    const uint32_t logicalChargerId = deviceId;

    if (signalName == "charger_id" || signalName == "slot_id")
        return static_cast<double>(logicalChargerId);

    if (signalName == "voltage" || signalName == "volt")
        return 12.0 + (counter % 30) * 0.05 + static_cast<double>(logicalChargerId) * 0.1;

    if (signalName == "cell_voltage")
        return 4.0 + (counter % 20) * 0.002 + static_cast<double>(logicalChargerId) * 0.01;

    if (signalName == "current")
        return -2.0 + (counter % 20) * 0.2;

    if (signalName == "cell_current")
        return -0.5 + (counter % 20) * 0.025;

    if (signalName == "temperature" || signalName == "temp")
        return 25.0 + (counter % 10) + static_cast<double>(logicalChargerId);

    if (signalName == "cell_temp")
        return 24.0 + (counter % 8) * 0.125 + static_cast<double>(logicalChargerId) * 0.1;

    if (signalName == "state" || signalName == "status")
        return counter % 3;

    if (signalName == "fault" || signalName == "fault_code")
        return (counter % 3 == 2) ? static_cast<double>((counter / 3) % 16) : 0.0;

    if (signalName == "enabled")
        return (counter % 2);

    if (signalName == "mode")
        return counter % 4;

    if (signalName == "setpoint")
        return 4.1 + (counter % 10) * 0.01;

    if (signalName == "start")
        return counter % 2;

    if (signalName == "speed")
        return 1000.0 + (counter % 50) * 10.0;

    if (signalName == "power")
        return 50.0 + (counter % 25) * 1.5;

    return static_cast<double>((counter + deviceId) % 100);
}

void SimulatedComsBackend::generateFakeMessage()
{
    if (!m_pcpDatabase || m_deviceIds.empty())
    {
        Logger::instance().logStatus("Simulated backend has no PCP devices");
        return;
    }

    ++m_counter;
    const uint32_t busDeviceId = m_deviceIds.front();
    const uint32_t logicalChargerId = static_cast<uint32_t>(((m_counter - 1) / 2) % 3) + 1u;
    const std::string messageName = (m_counter % 2 == 1) ? "status" : "cell_info";

    const PCPMessageDefinition* msgDef = m_pcpDatabase->messageByName(busDeviceId, messageName);

    if (!msgDef)
    {
        Logger::instance().logStatus(QString("COMS: message '%1' not found for device %2")
                                         .arg(QString::fromStdString(messageName))
                                         .arg(busDeviceId));
        return;
    }

    std::map<std::string, double> signalValues;
    for (const auto& [signalName, _] : msgDef->signalDefinitions)
    {
        signalValues[signalName] = fakeValueForSignal(signalName, logicalChargerId, m_counter);
    }

    PCPFrame frame = m_pcpEncoder.encode(busDeviceId, messageName, signalValues);
    publishFrame(frame, "RX", true);
}
