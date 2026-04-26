#include "safety_monitor.h"

#include <cmath>

SafetyDecision SafetyMonitor::evaluate(const SafetyInputs& inputs, const SafetyLimits& limits)
{
    SafetyDecision decision;
    auto addFault = [&decision](const QString& code)
    {
        decision.currentEnableAllowed = false;
        decision.faultCodes.push_back(code);
    };

    if (!inputs.selfTestPassed)
        addFault("CAL-06");
    if (inputs.emergencyStopActive)
        addFault("SAF-06");
    if (!inputs.chamberClosed)
        addFault("SAF-07");
    if (inputs.reversePolarityDetected)
        addFault("SAF-05");
    if (inputs.voltageV > limits.maxVoltageV)
        addFault("SAF-01");
    if (inputs.voltageV < limits.minVoltageV)
        addFault("SAF-02");
    if (std::abs(inputs.currentA) > limits.maxCurrentA)
        addFault("SAF-03");
    if (inputs.dutTemperatureDegC > limits.maxTemperatureDegC ||
        inputs.internalTemperatureDegC > limits.maxTemperatureDegC)
    {
        addFault("SAF-04");
    }

    return decision;
}
