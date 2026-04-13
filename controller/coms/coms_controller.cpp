#include "coms_controller.h"

#include "coms_backend.h"
#include "logger_backend.h"
#include "simulated_coms_backend.h"

ComsController::ComsController(Coms *view,
                               const PCPDatabase* pcpDatabase,
                               QObject *parent)
    : QObject(parent),
      m_view(view),
      m_pcpDatabase(pcpDatabase)
{
    connect(m_view, &Coms::addConnectionRequested,
            this, &ComsController::onAddConnectionRequested);
    connect(m_view, &Coms::connectConnectionRequested,
            this, &ComsController::onConnectConnectionRequested);
    connect(m_view, &Coms::disconnectConnectionRequested,
            this, &ComsController::onDisconnectConnectionRequested);
    connect(m_view, &Coms::removeConnectionRequested,
            this, &ComsController::onRemoveConnectionRequested);

    refreshView();
}

void ComsController::onTypeChanged(int)
{
}

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
            [this](const QString& message)
            {
                m_view->setStatusText(message);
            });

    connect(entry.backend, &IComsBackend::messageReceived, this,
            [this](const QString&)
            {
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
