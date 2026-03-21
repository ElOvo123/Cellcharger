#include "pcp_decoder.h"

#include <yaml-cpp/yaml.h>
#include <stdexcept>

bool PCPDecoder::loadFromFile(const std::string& yamlPath)
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

        PCPDecodeMessageDefinition def;
        def.messageId = msgNode["message_id"].as<uint32_t>();
        def.dlc = msgNode["dlc"].as<uint8_t>();

        const YAML::Node signals = msgNode["signals"];
        for (auto sigIt = signals.begin(); sigIt != signals.end(); ++sigIt)
        {
            const std::string signalName = sigIt->first.as<std::string>();
            const YAML::Node sigNode = sigIt->second;

            PCPDecodeSignalDefinition sig;
            sig.startBit = sigNode["start_bit"].as<int>();
            sig.bitLength = sigNode["bit_length"].as<int>();
            sig.isSigned = sigNode["signed"].as<bool>();
            sig.scale = sigNode["scale"].as<double>();
            sig.offset = sigNode["offset"].as<double>();

            def.signalDefinitions[signalName] = sig;
        }

        m_messages[name] = def;
    }

    return true;
}

std::optional<PCPDecodedMessage> PCPDecoder::decode(uint32_t canId, uint8_t dlc, const std::array<uint8_t, 8>& data) const
{
    PCPDecodedMessage out;
    out.canId = canId;
    out.deviceId = extractDeviceId(canId);
    out.messageId = extractMessageId(canId);
    out.dlc = dlc;

    std::string messageName;
    const PCPDecodeMessageDefinition* def =
        findMessageDefinition(out.messageId, messageName);

    if (!def)
        return std::nullopt;

    out.messageName = messageName;

    for (const auto& [signalName, sigDef] : def->signalDefinitions)
    {
        const uint64_t rawUnsigned = unpackBits(data, sigDef.startBit, sigDef.bitLength);

        const int64_t rawSigned = sigDef.isSigned ? rawToSigned(rawUnsigned, sigDef.bitLength) : static_cast<int64_t>(rawUnsigned);

        PCPDecodedSignal decoded;
        decoded.rawValue = rawSigned;
        decoded.physicalValue = rawToPhysical(rawSigned, sigDef);

        out.decodedSignals[signalName] = decoded;
    }

    return out;
}

uint32_t PCPDecoder::extractDeviceId(uint32_t canId) const
{
    return canId >> m_idLayout.messageIdBits;
}

uint32_t PCPDecoder::extractMessageId(uint32_t canId) const
{
    const uint32_t mask = (1u << m_idLayout.messageIdBits) - 1u;
    return canId & mask;
}

const PCPDecodeMessageDefinition* PCPDecoder::findMessageDefinition(uint32_t messageId,std::string& messageName) const
{
    for (const auto& [name, def] : m_messages)
    {
        if (def.messageId == messageId)
        {
            messageName = name;
            return &def;
        }
    }

    return nullptr;
}

uint64_t PCPDecoder::unpackBits(const std::array<uint8_t, 8>& data, int startBit, int bitLength)
{
    uint64_t value = 0;

    for (int i = 0; i < bitLength; ++i)
    {
        const int bitIndex = startBit + i;
        const int byteIndex = bitIndex / 8;
        const int bitInByte = bitIndex % 8;

        const bool bitSet = (data[byteIndex] >> bitInByte) & 0x1u;
        if (bitSet)
            value |= (1ULL << i);
    }

    return value;
}

int64_t PCPDecoder::rawToSigned(uint64_t rawValue, int bitLength)
{
    const uint64_t signBit = 1ULL << (bitLength - 1);

    if (rawValue & signBit)
    {
        const uint64_t mask = (1ULL << bitLength) - 1ULL;
        rawValue = (~rawValue + 1ULL) & mask;
        return -static_cast<int64_t>(rawValue);
    }

    return static_cast<int64_t>(rawValue);
}

double PCPDecoder::rawToPhysical(
    int64_t rawValue,
    const PCPDecodeSignalDefinition& signal)
{
    return static_cast<double>(rawValue) * signal.scale + signal.offset;
}