#pragma once

#include "system_requirements.h"

#include <QString>
#include <vector>

struct MeasurementSample
{
    double timestampSeconds = 0.0;
    double voltageV = 0.0;
    double currentA = 0.0;
    double temperatureDegC = 0.0;
    double pressureBar = 1.0;
};

struct IntegrationState
{
    bool valid = true;
    QString error;
    double capacityAh = 0.0;
    double energyWh = 0.0;
    double lastTimestampSeconds = 0.0;
};

struct InternalResistanceResult
{
    bool valid = false;
    QString error;
    double resistanceMilliOhm = 0.0;
};

class CapacityIntegrator
{
public:
    explicit CapacityIntegrator(double maxStepSeconds = SystemRequirements::integrationUpdateMs / 1000.0);

    void reset();
    IntegrationState update(const MeasurementSample& sample);
    IntegrationState state() const;

private:
    bool m_hasSample = false;
    double m_maxStepSeconds = 0.1;
    MeasurementSample m_lastSample;
    IntegrationState m_state;
};

class InternalResistanceCalculator
{
public:
    static bool pulseParametersValid(double pulseCurrentA, double pulseWidthMs, QString* errorMessage = nullptr);
    static InternalResistanceResult compute(double voltageBeforeV, double voltageDuringV, double currentBeforeA,
                                            double currentDuringA);
};
