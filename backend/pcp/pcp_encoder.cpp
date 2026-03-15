#include "pcp_encoder.h"

#include <yaml-cpp/yaml.h>
#include <cmath>
#include <stdexcept>

bool PCPEncoder::loadFromFile(const std::string& yamlPath)
{
    YAML::Node root = YAML::LoadFile(yamlPath);

    const YAML::Node idLayout = root["id_layout"];
    m_idLayout.totalBits = idLayout["total_bits"].as<int>();
    m_idLayout.deviceIdBits = idLayout["device_id_bits"].as<int>();
    m_idLayout.messageIdBits = idLayout["message_id_bits"].as<int>();

    m_messages.clear();

    const YAML::Node messages = root["messages"];
    for (auto it = messages.begin(); it != messages.end(); ++it)
    {
        const std::string name = it->first.as<std::string>();
        const YAML::Node msgNode = it->second;

        PCPMessageDefinition def;
        def.messageId = msgNode["message_id"].as<uint32_t>();
        def.dlc = msgNode["dlc"].as<uint8_t>();

        const YAML::Node signalsDefenitions = msgNode["signals"];
        for (auto sigIt = signalsDefenitions.begin(); sigIt != signalsDefenitions.end(); ++sigIt)
        {
            const std::string signalName = sigIt->first.as<std::string>();
            const YAML::Node sigNode = sigIt->second;

            PCPSignalDefinition sig;
            sig.startBit = sigNode["start_bit"].as<int>();
            sig.bitLength = sigNode["bit_length"].as<int>();
            sig.isSigned = sigNode["signed"].as<bool>();
            sig.scale = sigNode["scale"].as<double>();
            sig.offset = sigNode["offset"].as<double>();

            def.signalsDefinitions[signalName] = sig;
        }

        m_messages[name] = def;
    }

    return true;
}

PCPFrame PCPEncoder::encode(
    const std::string& messageName,
    uint32_t deviceId,
    const std::map<std::string, double>& signalValues) const
{
    auto msgIt = m_messages.find(messageName);
    if (msgIt == m_messages.end())
        throw std::runtime_error("Unknown PCP message: " + messageName);

    const PCPMessageDefinition& def = msgIt->second;

    PCPFrame frame;
    frame.id = buildId(deviceId, def.messageId);
    frame.dlc = def.dlc;
    frame.data.fill(0);

    for (const auto& [signalName, physicalValue] : signalValues)
    {
        auto sigIt = def.signalsDefinitions.find(signalName);
        if (sigIt == def.signalsDefinitions.end())
            throw std::runtime_error("Unknown PCP signal: " + signalName);

        const PCPSignalDefinition& sig = sigIt->second;
        const uint64_t raw = physicalToRaw(physicalValue, sig);

        packBits(frame.data, raw, sig.startBit, sig.bitLength);
    }

    return frame;
}

uint32_t PCPEncoder::buildId(uint32_t deviceId, uint32_t messageId) const
{
    const uint32_t maxDeviceId = (1u << m_idLayout.deviceIdBits) - 1u;
    const uint32_t maxMessageId = (1u << m_idLayout.messageIdBits) - 1u;

    if (deviceId > maxDeviceId)
        throw std::runtime_error("PCP device_id out of range");

    if (messageId > maxMessageId)
        throw std::runtime_error("PCP message_id out of range");

    return (deviceId << m_idLayout.messageIdBits) | messageId;
}

uint64_t PCPEncoder::physicalToRaw(
    double physicalValue,
    const PCPSignalDefinition& signal)
{
    const double rawDouble = (physicalValue - signal.offset) / signal.scale;
    int64_t rawSigned = static_cast<int64_t>(std::llround(rawDouble));

    if (signal.isSigned)
    {
        const int64_t minVal = -(1LL << (signal.bitLength - 1));
        const int64_t maxVal =  (1LL << (signal.bitLength - 1)) - 1;

        if (rawSigned < minVal || rawSigned > maxVal)
            throw std::runtime_error("PCP signed signal out of range");

        if (rawSigned < 0)
        {
            const uint64_t mask = (1ULL << signal.bitLength) - 1ULL;
            return static_cast<uint64_t>(rawSigned) & mask;
        }

        return static_cast<uint64_t>(rawSigned);
    }

    if (rawSigned < 0)
        throw std::runtime_error("PCP unsigned signal cannot be negative");

    const uint64_t maxVal = (1ULL << signal.bitLength) - 1ULL;
    if (static_cast<uint64_t>(rawSigned) > maxVal)
        throw std::runtime_error("PCP unsigned signal out of range");

    return static_cast<uint64_t>(rawSigned);
}

void PCPEncoder::packBits(
    std::array<uint8_t, 8>& data,
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