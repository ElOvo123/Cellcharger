#include "pcp_encoder.h"

#include <cmath>
#include <stdexcept>

PCPEncoder::PCPEncoder(const PCPDatabase* database)
    : m_database(database)
{
}

void PCPEncoder::setDatabase(const PCPDatabase* database)
{
    m_database = database;
}

const PCPDatabase* PCPEncoder::database() const
{
    return m_database;
}

PCPFrame PCPEncoder::encode(uint32_t deviceId,
                            const std::string& messageName,
                            const std::map<std::string, double>& signalValues) const
{
    if (!m_database)
        throw std::runtime_error("PCPEncoder has no database");

    const PCPMessageDefinition* msgDef = m_database->messageByName(deviceId, messageName);
    if (!msgDef)
        throw std::runtime_error("Unknown message" + messageName + " for device" + std::to_string(deviceId));

    PCPFrame frame;
    frame.id = buildId(deviceId, msgDef->messageId);
    frame.dlc = msgDef->dlc;
    frame.data.fill(0);

    for (const auto& [signalName, physicalValue] : signalValues)
    {
        auto sigIt = msgDef->signalDefinitions.find(signalName);
        if (sigIt == msgDef->signalDefinitions.end())
            throw std::runtime_error("Unknown signal for device/message");

        const uint64_t raw = physicalToRaw(physicalValue, sigIt->second);
        packBits(frame.data, raw, sigIt->second.startBit, sigIt->second.bitLength);
    }

    return frame;
}

uint32_t PCPEncoder::buildId(uint32_t deviceId, uint32_t messageId) const
{
    const PCPIdLayout& layout = m_database->idLayout();

    const uint32_t maxDeviceId = (1u << layout.deviceIdBits) - 1u;
    const uint32_t maxMessageId = (1u << layout.messageIdBits) - 1u;

    if (deviceId > maxDeviceId)
        throw std::runtime_error("device_id out of range");

    if (messageId > maxMessageId)
        throw std::runtime_error("message_id out of range");

    return (deviceId << layout.messageIdBits) | messageId;
}

uint64_t PCPEncoder::physicalToRaw(double physicalValue,
                                   const PCPSignalDefinition& signal)
{
    const double rawDouble = (physicalValue - signal.offset) / signal.scale;
    int64_t rawSigned = static_cast<int64_t>(std::llround(rawDouble));

    if (signal.isSigned)
    {
        const int64_t minVal = -(1LL << (signal.bitLength - 1));
        const int64_t maxVal =  (1LL << (signal.bitLength - 1)) - 1;

        if (rawSigned < minVal || rawSigned > maxVal)
            throw std::runtime_error("Signed signal out of range");

        if (rawSigned < 0)
        {
            const uint64_t mask = (1ULL << signal.bitLength) - 1ULL;
            return static_cast<uint64_t>(rawSigned) & mask;
        }

        return static_cast<uint64_t>(rawSigned);
    }

    if (rawSigned < 0)
        throw std::runtime_error("Unsigned signal cannot be negative");

    const uint64_t maxVal = (1ULL << signal.bitLength) - 1ULL;
    if (static_cast<uint64_t>(rawSigned) > maxVal)
        throw std::runtime_error("Unsigned signal out of range");

    return static_cast<uint64_t>(rawSigned);
}

void PCPEncoder::packBits(std::array<uint8_t, 8>& data,
                          uint64_t rawValue,
                          int startBit,
                          int bitLength)
{
    for (int i = 0; i < bitLength; ++i)
    {
        const int bitIndex = startBit + i;
        const int byteIndex = bitIndex / 8;
        const int bitInByte = bitIndex % 8;

        const bool bitSet = (rawValue >> i) & 0x1ULL;

        if (bitSet)
            data[byteIndex] |= static_cast<uint8_t>(1u << bitInByte);
        else
            data[byteIndex] &= static_cast<uint8_t>(~(1u << bitInByte));
    }
}