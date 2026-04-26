#pragma once

#include <QString>
#include <vector>

struct SafetyLimits
{
    double maxVoltageV = 5.5;
    double minVoltageV = 0.0;
    double maxCurrentA = 110.0;
    double maxTemperatureDegC = 60.0;
};

struct SafetyInputs
{
    double voltageV = 0.0;
    double currentA = 0.0;
    double dutTemperatureDegC = 25.0;
    double internalTemperatureDegC = 25.0;
    bool chamberClosed = true;
    bool emergencyStopActive = false;
    bool reversePolarityDetected = false;
    bool selfTestPassed = true;
};

struct SafetyDecision
{
    bool currentEnableAllowed = true;
    std::vector<QString> faultCodes;
};

class SafetyMonitor
{
public:
    static SafetyDecision evaluate(const SafetyInputs& inputs, const SafetyLimits& limits = {});
};
