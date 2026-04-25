#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

struct PCPIdLayout
{
    int totalBits = 11;
    int deviceIdBits = 4;
    int messageIdBits = 7;
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
    std::string name;
    uint32_t messageId = 0;
    uint8_t dlc = 8;
    std::map<std::string, PCPSignalDefinition> signalDefinitions;
};

struct PCPDeviceDefinition
{
    uint32_t deviceId = 0;
    std::string name;
    std::map<std::string, PCPMessageDefinition> messagesByName;
};

class PCPDatabase
{
public:
    bool loadFromFile(const std::string& yamlPath);

    const PCPIdLayout& idLayout() const;

    std::optional<PCPDeviceDefinition> device(uint32_t deviceId) const;
    std::string deviceName(uint32_t deviceId) const;
    std::vector<uint32_t> deviceIds() const;

    const PCPMessageDefinition* messageByName(uint32_t deviceId, const std::string& messageName) const;

    const PCPMessageDefinition* messageById(uint32_t deviceId, uint32_t messageId) const;

    std::vector<std::string> signalNames(uint32_t deviceId, const std::string& messageName) const;

    std::vector<std::string> messageNames(uint32_t deviceId) const;

private:
    PCPIdLayout m_idLayout;
    std::map<uint32_t, PCPDeviceDefinition> m_devices;
};