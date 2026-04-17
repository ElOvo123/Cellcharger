#pragma once

#include "icomms_backend.h"
#include "../pcp/pcp_database.h"
#include "../pcp/pcp_encoder.h"
#include "../pcp/pcp_decoder.h"

#include <QTimer>
#include <map>
#include <vector>

class SimulatedComsBackend : public IComsBackend
{
    Q_OBJECT

public:
    explicit SimulatedComsBackend(const PCPDatabase* db, QObject *parent = nullptr);

    void setType(ComsType type) override;
    void setConfig(const ComsConfig &config) override;
    void connectTransport() override;
    void disconnectTransport() override;
    bool sendFrame(const PCPFrame& frame, const PCPDatabase& database) override;

private slots:
    void generateFakeMessage();

private:
    void setState(State state);
    void publishFrame(const PCPFrame& frame,
                      const QString& direction,
                      bool decodeFrame);

    double fakeValueForSignal(const std::string& signalName,
                              uint32_t deviceId,
                              int counter) const;

private:
    const PCPDatabase* m_pcpDatabase = nullptr;

    ComsType m_type = ComsType::Serial;
    ComsConfig m_config;
    State m_state = State::Disconnected;
    QTimer m_timer;
    int m_counter = 0;

    PCPEncoder m_pcpEncoder;
    PCPDecoder m_pcpDecoder;

    std::vector<uint32_t> m_deviceIds;
    int m_deviceIndex = 0;

    std::map<uint32_t, int> m_messageIndexByDevice;
};
