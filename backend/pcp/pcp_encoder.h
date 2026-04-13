#pragma once

#include "pcp_database.h"

#include <array>
#include <cstdint>
#include <map>
#include <string>

struct PCPFrame
{
    uint32_t id = 0;
    uint8_t dlc = 0;
    std::array<uint8_t, 8> data{};
};

class PCPEncoder
{
public:
    explicit PCPEncoder(const PCPDatabase* database = nullptr);

    void setDatabase(const PCPDatabase* database);
    const PCPDatabase* database() const;

    PCPFrame encode(uint32_t deviceId,
                    const std::string& messageName,
                    const std::map<std::string, double>& signalValues) const;

private:
    const PCPDatabase* m_database = nullptr;

    uint32_t buildId(uint32_t deviceId, uint32_t messageId) const;

    static uint64_t physicalToRaw(double physicalValue,
                                  const PCPSignalDefinition& signal);

    static void packBits(std::array<uint8_t, 8>& data,
                         uint64_t rawValue,
                         int startBit,
                         int bitLength);
};