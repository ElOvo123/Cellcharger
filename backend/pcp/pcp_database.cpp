#include "pcp_database.h"

#include <yaml-cpp/yaml.h>

bool PCPDatabase::loadFromFile(const std::string& yamlPath)
{
    YAML::Node root = YAML::LoadFile(yamlPath);

    const YAML::Node idLayout = root["id_layout"];
    m_idLayout.totalBits = idLayout["total_bits"].as<int>();
    m_idLayout.deviceIdBits = idLayout["device_id_bits"].as<int>();
    m_idLayout.messageIdBits = idLayout["message_id_bits"].as<int>();

    m_devices.clear();

    const YAML::Node devices = root["devices"];
    if (!devices)
        return true;

    for (auto devIt = devices.begin(); devIt != devices.end(); ++devIt)
    {
        const uint32_t deviceId = devIt->first.as<uint32_t>();
        const YAML::Node devNode = devIt->second;

        PCPDeviceDefinition deviceDef;
        deviceDef.deviceId = deviceId;
        deviceDef.name = devNode["name"].as<std::string>();

        const YAML::Node messages = devNode["messages"];
        if (messages)
        {
            for (auto msgIt = messages.begin(); msgIt != messages.end(); ++msgIt)
            {
                const std::string messageName = msgIt->first.as<std::string>();
                const YAML::Node msgNode = msgIt->second;

                PCPMessageDefinition msgDef;
                msgDef.name = messageName;
                msgDef.messageId = msgNode["message_id"].as<uint32_t>();
                msgDef.dlc = msgNode["dlc"].as<uint8_t>();

                const YAML::Node signals = msgNode["signals"];
                if (signals)
                {
                    for (auto sigIt = signals.begin(); sigIt != signals.end(); ++sigIt)
                    {
                        const std::string signalName = sigIt->first.as<std::string>();
                        const YAML::Node sigNode = sigIt->second;

                        PCPSignalDefinition sigDef;
                        sigDef.startBit = sigNode["start_bit"].as<int>();
                        sigDef.bitLength = sigNode["bit_length"].as<int>();
                        sigDef.isSigned = sigNode["signed"].as<bool>();
                        sigDef.scale = sigNode["scale"].as<double>();
                        sigDef.offset = sigNode["offset"].as<double>();

                        msgDef.signalDefinitions[signalName] = sigDef;
                    }
                }

                deviceDef.messagesByName[messageName] = msgDef;
            }
        }

        m_devices[deviceId] = deviceDef;
    }

    return true;
}

const PCPIdLayout& PCPDatabase::idLayout() const
{
    return m_idLayout;
}

std::optional<PCPDeviceDefinition> PCPDatabase::device(uint32_t deviceId) const
{
    auto it = m_devices.find(deviceId);
    if (it == m_devices.end())
        return std::nullopt;

    return it->second;
}

std::string PCPDatabase::deviceName(uint32_t deviceId) const
{
    auto it = m_devices.find(deviceId);
    if (it == m_devices.end())
        return "Device " + std::to_string(deviceId);

    return it->second.name;
}

std::vector<uint32_t> PCPDatabase::deviceIds() const
{
    std::vector<uint32_t> ids;
    ids.reserve(m_devices.size());

    for (const auto& [deviceId, _] : m_devices)
        ids.push_back(deviceId);

    return ids;
}

const PCPMessageDefinition* PCPDatabase::messageByName(uint32_t deviceId,
                                                       const std::string& messageName) const
{
    auto devIt = m_devices.find(deviceId);
    if (devIt == m_devices.end())
        return nullptr;

    auto msgIt = devIt->second.messagesByName.find(messageName);
    if (msgIt == devIt->second.messagesByName.end())
        return nullptr;

    return &msgIt->second;
}

const PCPMessageDefinition* PCPDatabase::messageById(uint32_t deviceId,
                                                     uint32_t messageId) const
{
    auto devIt = m_devices.find(deviceId);
    if (devIt == m_devices.end())
        return nullptr;

    for (const auto& [_, msgDef] : devIt->second.messagesByName)
    {
        if (msgDef.messageId == messageId)
            return &msgDef;
    }

    return nullptr;
}

std::vector<std::string> PCPDatabase::signalNames(uint32_t deviceId,
                                                  const std::string& messageName) const
{
    std::vector<std::string> names;

    const PCPMessageDefinition* msg = messageByName(deviceId, messageName);
    if (!msg)
        return names;

    for (const auto& [signalName, _] : msg->signalDefinitions)
        names.push_back(signalName);

    return names;
}

std::vector<std::string> PCPDatabase::messageNames(uint32_t deviceId) const
{
    std::vector<std::string> names;

    auto devIt = m_devices.find(deviceId);
    if (devIt == m_devices.end())
        return names;

    for (const auto& [messageName, _] : devIt->second.messagesByName)
        names.push_back(messageName);

    return names;
}