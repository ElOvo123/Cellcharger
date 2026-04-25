#include "coms_controller.h"

#include "coms_backend.h"
#include "logger_backend.h"
#include "simulated_coms_backend.h"

#include <cmath>
#include <iostream>

namespace
{
constexpr qint64 kTelemetryFreshTimeoutMs = 2000;

bool physicalValueFitsSignal(double physicalValue, const PCPSignalDefinition& signal)
{
    if (!std::isfinite(physicalValue) || !std::isfinite(signal.scale) || signal.scale == 0.0 ||
        !std::isfinite(signal.offset))
    {
        return false;
    }

    const double rawDouble = (physicalValue - signal.offset) / signal.scale;
    if (!std::isfinite(rawDouble))
        return false;

    const double rounded = std::round(rawDouble);

    if (signal.bitLength <= 0 || signal.bitLength >= 63)
        return false;

    if (signal.isSigned)
    {
        const double minVal = -static_cast<double>(1LL << (signal.bitLength - 1));
        const double maxVal = static_cast<double>((1LL << (signal.bitLength - 1)) - 1);
        return rounded >= minVal && rounded <= maxVal;
    }

    const double maxVal = static_cast<double>((1ULL << signal.bitLength) - 1ULL);
    return rounded >= 0.0 && rounded <= maxVal;
}
} // namespace

ComsController::ComsController(Coms* view, const PCPDatabase* pcpDatabase, QObject* parent)
    : QObject(parent), m_view(view), m_pcpDatabase(pcpDatabase)
{
    connect(m_view, &Coms::addConnectionRequested, this, &ComsController::onAddConnectionRequested);
    connect(m_view, &Coms::connectConnectionRequested, this, &ComsController::onConnectConnectionRequested);
    connect(m_view, &Coms::disconnectConnectionRequested, this, &ComsController::onDisconnectConnectionRequested);
    connect(m_view, &Coms::removeConnectionRequested, this, &ComsController::onRemoveConnectionRequested);

    refreshView();
}

void ComsController::onTypeChanged(int) {}

void ComsController::onAddConnectionRequested()
{
    ConnectionEntry entry;
    entry.type = ComsType::Serial;
    entry.config.baudrate = 115200;
    entry.backend = createBackend();

    connect(entry.backend, &IComsBackend::stateChanged, this,
            [this, backend = entry.backend](IComsBackend::State state)
            {
                for (ConnectionEntry& item : m_connections)
                {
                    if (item.backend != backend)
                        continue;

                    item.state = state;
                    refreshView();
                    refreshOverallIndicator();
                    return;
                }
            });

    connect(entry.backend, &IComsBackend::statusMessage, this,
            [this](const QString& message) { m_view->setStatusText(message); });

    connect(entry.backend, &IComsBackend::messageReceived, this,
            [this](const QString&)
            {
                m_hasReceivedTelemetry = true;
                m_lastRxTimer.restart();
                m_view->pulseReceiveActivity();
                refreshOverallIndicator();
            });

    m_connections.append(entry);
    refreshView();
    m_view->setStatusText("Connection added");
}

void ComsController::onConnectConnectionRequested(int row)
{
    if (row < 0 || row >= m_connections.size())
        return;

    ConnectionEntry& entry = m_connections[row];
    entry.type = m_view->connectionType(row);
    entry.config = m_view->connectionConfig(row);
    entry.backend->setType(entry.type);
    entry.backend->setConfig(entry.config);
    entry.backend->connectTransport();
    refreshView();
}

void ComsController::onDisconnectConnectionRequested(int row)
{
    if (row < 0 || row >= m_connections.size())
        return;

    m_connections[row].backend->disconnectTransport();
}

void ComsController::onRemoveConnectionRequested(int row)
{
    if (row < 0 || row >= m_connections.size())
        return;

    ConnectionEntry entry = m_connections.takeAt(row);
    if (entry.backend)
    {
        entry.backend->disconnectTransport();
        entry.backend->deleteLater();
    }

    refreshView();
    refreshOverallIndicator();
    m_view->setStatusText("Connection removed");
}

bool ComsController::hasFreshTelemetry() const
{
    return m_hasReceivedTelemetry && m_lastRxTimer.isValid() && m_lastRxTimer.elapsed() <= kTelemetryFreshTimeoutMs;
}

IComsBackend* ComsController::createBackend() const
{
#ifdef USE_SIM_COMS
    return new SimulatedComsBackend(m_pcpDatabase, const_cast<ComsController*>(this));
#else
    return new ComsBackend(const_cast<ComsController*>(this));
#endif
}

QString ComsController::transportName(ComsType type) const
{
    switch (type)
    {
        case ComsType::Serial:
            return "Serial";
        case ComsType::Socket_vcan:
            return "Socket vcan";
        case ComsType::Socket_UDP:
            return "Socket UDP";
        case ComsType::Socket_TCP:
            return "Socket TCP";
    }

    return "Unknown";
}

void ComsController::refreshView()
{
    QList<ComsConnectionInfo> rows;
    rows.reserve(m_connections.size());

    for (int i = 0; i < m_connections.size(); ++i)
    {
        ConnectionEntry& entry = m_connections[i];
        entry.type = m_view->connectionType(i);
        entry.config = m_view->connectionConfig(i);

        ComsConnectionInfo row;
        row.type = entry.type;
        row.config = entry.config;
        row.connected = entry.state == IComsBackend::State::Connected;

        switch (entry.state)
        {
            case IComsBackend::State::Disconnected:
                row.indicatorColor = QColor("#d6b63f");
                break;
            case IComsBackend::State::Connecting:
                row.indicatorColor = QColor("#8aa7d8");
                break;
            case IComsBackend::State::Connected:
                row.indicatorColor = QColor("#4caf50");
                break;
            case IComsBackend::State::Error:
                row.indicatorColor = QColor("#d97b5c");
                break;
        }

        rows.append(row);
    }

    m_view->setConnections(rows);
}

void ComsController::refreshOverallIndicator()
{
    bool hasConnected = false;
    for (const ConnectionEntry& entry : m_connections)
    {
        if (entry.state != IComsBackend::State::Connected)
            continue;

        hasConnected = true;
        break;
    }

    m_view->setOverallConnected(hasConnected);
}

bool ComsController::validateChargerCommand(uint32_t chargerId, int mode, bool start, double setpoint,
                                            QString* errorMessage) const
{
    if (!m_pcpDatabase)
    {
        if (errorMessage)
            *errorMessage = "no PCP database available";
        return false;
    }

    if (!std::isfinite(setpoint))
    {
        if (errorMessage)
            *errorMessage = "setpoint must be finite";
        return false;
    }

    const PCPMessageDefinition* commandMessage = m_pcpDatabase->messageByName(1, "command");
    if (!commandMessage)
    {
        if (errorMessage)
            *errorMessage = "command message is not defined";
        return false;
    }

    const std::map<std::string, double> values = {{"charger_id", static_cast<double>(chargerId)},
                                                  {"mode", static_cast<double>(mode)},
                                                  {"setpoint", setpoint},
                                                  {"start", start ? 1.0 : 0.0}};

    for (const auto& [signalName, value] : values)
    {
        const auto signalIt = commandMessage->signalDefinitions.find(signalName);
        if (signalIt == commandMessage->signalDefinitions.end())
        {
            if (errorMessage)
                *errorMessage = QString("command signal '%1' is not defined").arg(QString::fromStdString(signalName));
            return false;
        }

        if (!physicalValueFitsSignal(value, signalIt->second))
        {
            if (errorMessage)
                *errorMessage = QString("command signal '%1' is out of range").arg(QString::fromStdString(signalName));
            return false;
        }
    }

    return true;
}

bool ComsController::validateEncodedFrame(const PCPFrame& frame, QString* errorMessage) const
{
    if (!m_pcpDatabase)
    {
        if (errorMessage)
            *errorMessage = "no PCP database available";
        return false;
    }

    if (frame.dlc > frame.data.size())
    {
        if (errorMessage)
            *errorMessage = "frame DLC exceeds payload storage";
        return false;
    }

    const PCPIdLayout& layout = m_pcpDatabase->idLayout();
    if (layout.messageIdBits <= 0 || layout.messageIdBits >= 31)
    {
        if (errorMessage)
            *errorMessage = "invalid PCP id layout";
        return false;
    }

    const uint32_t messageMask = (1u << layout.messageIdBits) - 1u;
    const uint32_t deviceId = frame.id >> layout.messageIdBits;
    const uint32_t messageId = frame.id & messageMask;

    if (!m_pcpDatabase->device(deviceId).has_value())
    {
        if (errorMessage)
            *errorMessage = QString("unknown device id %1").arg(deviceId);
        return false;
    }

    const PCPMessageDefinition* message = m_pcpDatabase->messageById(deviceId, messageId);
    if (!message)
    {
        if (errorMessage)
            *errorMessage = QString("unknown message id %1 for device %2").arg(messageId).arg(deviceId);
        return false;
    }

    if (frame.dlc != message->dlc)
    {
        if (errorMessage)
            *errorMessage = QString("frame DLC %1 does not match message DLC %2").arg(frame.dlc).arg(message->dlc);
        return false;
    }

    return true;
}

bool ComsController::sendChargerCommand(uint32_t chargerId, int mode, bool start, double setpoint)
{
    if (!m_pcpDatabase)
    {
        Logger::instance().logStatus("COMS: no PCP database available for charger command");
        return false;
    }

    QString validationError;
    if (!validateChargerCommand(chargerId, mode, start, setpoint, &validationError))
    {
        Logger::instance().logStatus(QString("COMS: rejected charger command: %1").arg(validationError));
        return false;
    }

    PCPEncoder encoder(m_pcpDatabase);
    PCPFrame frame;
    try
    {
        frame = encoder.encode(1, "command",
                               {{"charger_id", static_cast<double>(chargerId)},
                                {"mode", static_cast<double>(mode)},
                                {"setpoint", setpoint},
                                {"start", start ? 1.0 : 0.0}});
    }
    catch (const std::exception& ex)
    {
        Logger::instance().logStatus(QString("COMS: failed to encode charger command: %1").arg(ex.what()));
        return false;
    }

    if (!validateEncodedFrame(frame, &validationError))
    {
        Logger::instance().logStatus(QString("COMS: rejected encoded charger frame: %1").arg(validationError));
        return false;
    }

    std::cout << "Status command: charger_id=" << chargerId << " mode=" << mode << " start=" << (start ? 1 : 0)
              << " setpoint=" << setpoint << std::endl;

    bool hasConnectedBackend = false;
    for (ConnectionEntry& entry : m_connections)
    {
        if (!entry.backend || entry.state != IComsBackend::State::Connected)
            continue;

        hasConnectedBackend = true;
    }

    if (!hasConnectedBackend)
    {
        Logger::instance().logStatus("COMS: no connected backend available to send charger command");
        return false;
    }

    if (start && !hasFreshTelemetry())
    {
        Logger::instance().logStatus("COMS: rejected charger command: telemetry is stale");
        return false;
    }

    for (ConnectionEntry& entry : m_connections)
    {
        if (!entry.backend || entry.state != IComsBackend::State::Connected)
            continue;

        if (entry.backend->sendFrame(frame, *m_pcpDatabase))
            return true;
    }

    Logger::instance().logStatus("COMS: no connected backend available to send charger command");
    return false;
}
