#include "pcp_decoder.h"

PCPDecoder::PCPDecoder(const PCPDatabase* database) : m_database(database) {}

void PCPDecoder::setDatabase(const PCPDatabase* database)
{
    m_database = database;
}

const PCPDatabase* PCPDecoder::database() const
{
    return m_database;
}

std::optional<PCPDecodedMessage> PCPDecoder::decode(uint32_t canId, uint8_t dlc,
                                                    const std::array<uint8_t, 8>& data) const
{
    if (!m_database)
        return std::nullopt;

    PCPDecodedMessage out;
    out.canId = canId;
    out.deviceId = extractDeviceId(canId);
    out.deviceName = m_database->deviceName(out.deviceId);
    out.messageId = extractMessageId(canId);
    out.dlc = dlc;

    const PCPMessageDefinition* msgDef = m_database->messageById(out.deviceId, out.messageId);
    if (!msgDef)
        return std::nullopt;

    out.messageName = msgDef->name;

    for (const auto& [signalName, sigDef] : msgDef->signalDefinitions)
    {
        const uint64_t rawUnsigned = unpackBits(data, sigDef.startBit, sigDef.bitLength);
        const int64_t rawSigned =
            sigDef.isSigned ? rawToSigned(rawUnsigned, sigDef.bitLength) : static_cast<int64_t>(rawUnsigned);

        PCPDecodedSignal decoded;
        decoded.rawValue = rawSigned;
        decoded.physicalValue = rawToPhysical(rawSigned, sigDef);

        out.decodedSignals[signalName] = decoded;
    }

    return out;
}

uint32_t PCPDecoder::extractDeviceId(uint32_t canId) const
{
    return canId >> m_database->idLayout().messageIdBits;
}

uint32_t PCPDecoder::extractMessageId(uint32_t canId) const
{
    const uint32_t mask = (1u << m_database->idLayout().messageIdBits) - 1u;
    return canId & mask;
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

double PCPDecoder::rawToPhysical(int64_t rawValue, const PCPSignalDefinition& signal)
{
    return static_cast<double>(rawValue) * signal.scale + signal.offset;
}