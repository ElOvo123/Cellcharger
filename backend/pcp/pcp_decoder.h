#pragma once

#include "pcp_database.h"

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>

struct PCPDecodedSignal
{
    double physicalValue = 0.0;
    int64_t rawValue = 0;
};

struct PCPDecodedMessage
{
    std::string messageName;
    uint32_t canId = 0;
    uint32_t deviceId = 0;
    std::string deviceName;
    uint32_t messageId = 0;
    uint8_t dlc = 0;
    std::map<std::string, PCPDecodedSignal> decodedSignals;
};

class PCPDecoder
{
public:
    explicit PCPDecoder(const PCPDatabase* database = nullptr);

    void setDatabase(const PCPDatabase* database);
    const PCPDatabase* database() const;

    std::optional<PCPDecodedMessage> decode(uint32_t canId,
                                            uint8_t dlc,
                                            const std::array<uint8_t, 8>& data) const;

private:
    const PCPDatabase* m_database = nullptr;

    uint32_t extractDeviceId(uint32_t canId) const;
    uint32_t extractMessageId(uint32_t canId) const;

    static uint64_t unpackBits(const std::array<uint8_t, 8>& data,
                               int startBit,
                               int bitLength);

    static int64_t rawToSigned(uint64_t rawValue, int bitLength);
    static double rawToPhysical(int64_t rawValue,
                                const PCPSignalDefinition& signal);
};