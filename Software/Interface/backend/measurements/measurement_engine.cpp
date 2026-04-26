#include "measurement_engine.h"

#include <cmath>

CapacityIntegrator::CapacityIntegrator(double maxStepSeconds) : m_maxStepSeconds(maxStepSeconds) {}

void CapacityIntegrator::reset()
{
    m_hasSample = false;
    m_lastSample = {};
    m_state = {};
}

IntegrationState CapacityIntegrator::update(const MeasurementSample& sample)
{
    if (!std::isfinite(sample.timestampSeconds) || !std::isfinite(sample.voltageV) || !std::isfinite(sample.currentA))
    {
        m_state.valid = false;
        m_state.error = "sample contains non-finite electrical value";
        return m_state;
    }

    if (!m_hasSample)
    {
        m_hasSample = true;
        m_lastSample = sample;
        m_state.lastTimestampSeconds = sample.timestampSeconds;
        return m_state;
    }

    const double deltaSeconds = sample.timestampSeconds - m_lastSample.timestampSeconds;
    if (deltaSeconds < 0.0)
    {
        m_state.valid = false;
        m_state.error = "sample timestamp moved backwards";
        return m_state;
    }

    if (deltaSeconds > m_maxStepSeconds)
    {
        m_state.valid = false;
        m_state.error = "integration step exceeds SRS limit";
        return m_state;
    }

    const double averageCurrentA = (m_lastSample.currentA + sample.currentA) / 2.0;
    const double averagePowerW =
        ((m_lastSample.voltageV * m_lastSample.currentA) + (sample.voltageV * sample.currentA)) / 2.0;
    m_state.capacityAh += averageCurrentA * deltaSeconds / 3600.0;
    m_state.energyWh += averagePowerW * deltaSeconds / 3600.0;
    m_state.lastTimestampSeconds = sample.timestampSeconds;
    m_lastSample = sample;
    return m_state;
}

IntegrationState CapacityIntegrator::state() const
{
    return m_state;
}

bool InternalResistanceCalculator::pulseParametersValid(double pulseCurrentA, double pulseWidthMs,
                                                        QString* errorMessage)
{
    if (pulseCurrentA < SystemRequirements::pulseCurrentRangeA.min ||
        pulseCurrentA > SystemRequirements::pulseCurrentRangeA.max)
    {
        if (errorMessage)
            *errorMessage = "pulse current is outside SRS range";
        return false;
    }

    if (pulseWidthMs < SystemRequirements::pulseWidthMs.min || pulseWidthMs > SystemRequirements::pulseWidthMs.max)
    {
        if (errorMessage)
            *errorMessage = "pulse width is outside SRS range";
        return false;
    }

    return true;
}

InternalResistanceResult InternalResistanceCalculator::compute(double voltageBeforeV, double voltageDuringV,
                                                               double currentBeforeA, double currentDuringA)
{
    InternalResistanceResult result;
    const double deltaCurrentA = currentDuringA - currentBeforeA;
    if (std::abs(deltaCurrentA) < SystemRequirements::pulseCurrentRangeA.min)
    {
        result.error = "current step is too small for IR measurement";
        return result;
    }

    const double deltaVoltageV = voltageBeforeV - voltageDuringV;
    result.resistanceMilliOhm = std::abs(deltaVoltageV / deltaCurrentA) * 1000.0;
    result.valid = result.resistanceMilliOhm >= SystemRequirements::internalResistanceMilliOhm.min &&
                   result.resistanceMilliOhm <= SystemRequirements::internalResistanceMilliOhm.max;
    if (!result.valid)
        result.error = "computed IR is outside SRS measurement range";
    return result;
}
