#pragma once

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

struct PCPSignalDefinition
{
    int startBit = 0;
    int bitLength = 0;
    bool isSigned = false;
    double scale = 1.0;
    double offset = 0.0;
};

struct PCPMessageDefinition
{
    uint32_t messageId = 0;
    uint8_t dlc = 8;
    std::map<std::string, PCPSignalDefinition> signalsDefinitions;
};

struct PCPIdLayout
{
    int totalBits = 11;
    int deviceIdBits = 4;
    int messageIdBits = 7;
};

class PCPEncoder
{
public:
    bool loadFromFile(const std::string& yamlPath);

    PCPFrame encode(
        const std::string& messageName,
        uint32_t deviceId,
        const std::map<std::string, double>& signalValues) const;

private:
    PCPIdLayout m_idLayout;
    std::map<std::string, PCPMessageDefinition> m_messages;

    uint32_t buildId(uint32_t deviceId, uint32_t messageId) const;

    static uint64_t physicalToRaw(
        double physicalValue,
        const PCPSignalDefinition& signal);

    static void packBits(
        std::array<uint8_t, 8>& data,
        uint64_t rawValue,
        int startBit,
        int bitLength);
};