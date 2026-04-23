#pragma once

#include <array>
#include <vector>

#include <QString>

#include "profile_plot_widget.h"

struct ProfileSlotDocument
{
    std::vector<Setpoint> setpoints;
    QString profileName;
    int displayModeIndex = 0;
};

struct ProfileDocument
{
    std::array<ProfileSlotDocument, 3> slotDocuments;
};

class ProfileYamlLogic
{
public:
    static QString serialize(const ProfileDocument& document);
    static bool deserialize(const QString& yamlText, ProfileDocument& document, QString* errorMessage = nullptr);
};
