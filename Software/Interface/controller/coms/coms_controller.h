#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QString>

#include "coms.h"
#include "icomms_backend.h"
#include "pcp_database.h"

struct PCPFrame;

class ComsController : public QObject
{
    Q_OBJECT

public:
    explicit ComsController(Coms* view, const PCPDatabase* pcpDatabase, QObject* parent = nullptr);

    bool sendChargerCommand(uint32_t chargerId, int mode, bool start, double setpoint);

private slots:
    void onTypeChanged(int index);
    void onAddConnectionRequested();
    void onConnectConnectionRequested(int row);
    void onDisconnectConnectionRequested(int row);
    void onRemoveConnectionRequested(int row);

private:
    struct ConnectionEntry
    {
        ComsType type = ComsType::Serial;
        ComsConfig config;
        IComsBackend* backend = nullptr;
        IComsBackend::State state = IComsBackend::State::Disconnected;
    };

    IComsBackend* createBackend() const;
    QString transportName(ComsType type) const;
    void refreshView();
    void refreshOverallIndicator();
    bool hasFreshTelemetry() const;
    bool validateChargerCommand(uint32_t chargerId, int mode, bool start, double setpoint, QString* errorMessage) const;
    bool validateEncodedFrame(const PCPFrame& frame, QString* errorMessage) const;

private:
    Coms* m_view;
    const PCPDatabase* m_pcpDatabase = nullptr;
    QList<ConnectionEntry> m_connections;
    QElapsedTimer m_lastRxTimer;
    bool m_hasReceivedTelemetry = false;
};
