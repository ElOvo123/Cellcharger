#pragma once

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
    uint32_t messageId = 0;
    uint8_t dlc = 0;
    std::map<std::string, PCPDecodedSignal> decodedSignals;
};

struct PCPDecodeSignalDefinition
{
    int startBit = 0;
    int bitLength = 0;
    bool isSigned = false;
    double scale = 1.0;
    double offset = 0.0;
};

struct PCPDecodeMessageDefinition
{
    uint32_t messageId = 0;
    uint8_t dlc = 8;
    std::map<std::string, PCPDecodeSignalDefinition> signalDefinitions;
};

struct PCPDecodeIdLayout
{
    int totalBits = 11;
    int deviceIdBits = 4;
    int messageIdBits = 7;
};

class PCPDecoder
{
public:
    bool loadFromFile(const std::string& yamlPath);

    std::optional<PCPDecodedMessage> decode(uint32_t canId, uint8_t dlc, const std::array<uint8_t, 8>& data) const;

private:
    PCPDecodeIdLayout m_idLayout;
    std::map<std::string, PCPDecodeMessageDefinition> m_messages;

    uint32_t extractDeviceId(uint32_t canId) const;
    uint32_t extractMessageId(uint32_t canId) const;

    const PCPDecodeMessageDefinition* findMessageDefinition(
        uint32_t messageId,
        std::string& messageName) const;

    static uint64_t unpackBits(
        const std::array<uint8_t, 8>& data,
        int startBit,
        int bitLength);

    static int64_t rawToSigned(uint64_t rawValue, int bitLength);
    static double rawToPhysical(
        int64_t rawValue,
        const PCPDecodeSignalDefinition& signal);
};