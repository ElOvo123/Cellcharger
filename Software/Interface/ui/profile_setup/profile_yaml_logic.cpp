#include "profile_yaml_logic.h"

#include <exception>
#include <string>

#include <yaml-cpp/yaml.h>

namespace
{
QString yamlStringValue(const YAML::Node& node, const char* key, const QString& fallback = QString())
{
    try
    {
        if (node[key])
            return QString::fromStdString(node[key].as<std::string>());
    }
    catch (...)
    {
    }
    return fallback;
}

int yamlIntValue(const YAML::Node& node, const char* key, int fallback = 0)
{
    try
    {
        if (node[key])
            return node[key].as<int>();
    }
    catch (...)
    {
    }
    return fallback;
}

double yamlDoubleValue(const YAML::Node& node, const char* key, double fallback = 0.0)
{
    try
    {
        if (node[key])
            return node[key].as<double>();
    }
    catch (...)
    {
    }
    return fallback;
}

void appendSlotYaml(YAML::Emitter& out, const ProfileSlotDocument& slot)
{
    out << YAML::BeginMap;
    out << YAML::Key << "setpoints" << YAML::Value << YAML::BeginSeq;

    for (const Setpoint& setpoint : slot.setpoints)
    {
        out << YAML::BeginMap;
        out << YAML::Key << "time" << YAML::Value << setpoint.time;
        out << YAML::Key << "voltage" << YAML::Value << setpoint.voltage;
        out << YAML::Key << "current" << YAML::Value << setpoint.current;
        out << YAML::Key << "temperature" << YAML::Value << setpoint.temperature;
        out << YAML::Key << "curveType" << YAML::Value << setpoint.curveType.toStdString();
        out << YAML::Key << "rampStep" << YAML::Value << setpoint.rampStep;
        out << YAML::EndMap;
    }

    out << YAML::EndSeq;
    out << YAML::EndMap;
}

void loadSlotFromYamlNode(const YAML::Node& slotNode, ProfileSlotDocument& slot)
{
    slot.setpoints.clear();
    if (!slotNode || !slotNode["setpoints"] || !slotNode["setpoints"].IsSequence())
        return;

    const YAML::Node setpointsNode = slotNode["setpoints"];
    for (std::size_t i = 0; i < setpointsNode.size(); ++i)
    {
        const YAML::Node point = setpointsNode[i];
        if (!point.IsMap())
            continue;

        Setpoint setpoint;
        setpoint.time = yamlDoubleValue(point, "time", 0.0);
        setpoint.voltage = yamlDoubleValue(point, "voltage", 0.0);
        setpoint.current = yamlDoubleValue(point, "current", 0.0);
        setpoint.temperature = yamlDoubleValue(point, "temperature", 0.0);
        setpoint.curveType = yamlStringValue(point, "curveType", "Ramp");
        setpoint.rampStep = yamlDoubleValue(point, "rampStep", 1.0);
        slot.setpoints.push_back(setpoint);
    }
}
}

QString ProfileYamlLogic::serialize(const ProfileDocument& document)
{
    YAML::Emitter out;
    out << YAML::BeginMap;

    for (size_t i = 0; i < document.slotDocuments.size(); ++i)
    {
        out << YAML::Key << QString("slot%1").arg(i + 1).toStdString() << YAML::Value;
        appendSlotYaml(out, document.slotDocuments[i]);
    }

    for (size_t i = 0; i < document.slotDocuments.size(); ++i)
    {
        out << YAML::Key << QString("profileName%1").arg(i + 1).toStdString()
            << YAML::Value << document.slotDocuments[i].profileName.toStdString();
    }

    for (size_t i = 0; i < document.slotDocuments.size(); ++i)
    {
        out << YAML::Key << QString("displayMode%1").arg(i + 1).toStdString()
            << YAML::Value << document.slotDocuments[i].displayModeIndex;
    }

    out << YAML::EndMap;
    return QString::fromStdString(out.c_str());
}

bool ProfileYamlLogic::deserialize(const QString& yamlText, ProfileDocument& document, QString* errorMessage)
{
    try
    {
        const YAML::Node root = YAML::Load(yamlText.toStdString());
        if (!root || !root.IsMap())
        {
            if (errorMessage)
                *errorMessage = "Invalid YAML format";
            return false;
        }

        for (size_t i = 0; i < document.slotDocuments.size(); ++i)
        {
            ProfileSlotDocument& slot = document.slotDocuments[i];
            loadSlotFromYamlNode(root[QString("slot%1").arg(i + 1).toStdString()], slot);
            slot.profileName = yamlStringValue(root, QString("profileName%1").arg(i + 1).toStdString().c_str(), slot.profileName);
            slot.displayModeIndex = yamlIntValue(root, QString("displayMode%1").arg(i + 1).toStdString().c_str(), 0);
        }

        return true;
    }
    catch (const std::exception& ex)
    {
        if (errorMessage)
            *errorMessage = ex.what();
        return false;
    }
}
