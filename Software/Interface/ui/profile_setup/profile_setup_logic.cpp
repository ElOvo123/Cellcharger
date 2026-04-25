#include "profile_setup_logic.h"

#include <algorithm>
#include <cmath>

ProfilePlotWidget::DisplayMode ProfileSetupLogic::displayModeForControlIndex(int controlModeIndex)
{
    switch (controlModeIndex)
    {
        case 1:
            return ProfilePlotWidget::DisplayMode::Voltage;
        case 2:
            return ProfilePlotWidget::DisplayMode::Current;
        case 3:
            return ProfilePlotWidget::DisplayMode::Temperature;
        default:
            return ProfilePlotWidget::DisplayMode::All;
    }
}

int ProfileSetupLogic::commandModeForControlIndex(int controlModeIndex)
{
    return controlModeIndex == 1 ? 1 : 0;
}

std::vector<Setpoint> ProfileSetupLogic::normalizedSetpoints(const std::vector<Setpoint>& setpoints)
{
    std::vector<Setpoint> normalized = setpoints;
    std::sort(normalized.begin(), normalized.end(),
              [](const Setpoint& lhs, const Setpoint& rhs) { return lhs.time < rhs.time; });

    if (!normalized.empty() && normalized.front().time > 0.0)
    {
        Setpoint baseline;
        baseline.time = 0.0;
        baseline.current = 0.0;
        baseline.voltage = normalized.front().voltage;
        baseline.temperature = 25.0;
        baseline.curveType = normalized.front().curveType;
        baseline.rampStep = normalized.front().rampStep;
        normalized.insert(normalized.begin(), baseline);
    }

    return normalized;
}

int ProfileSetupLogic::activeRowForElapsedSeconds(const std::vector<Setpoint>& setpoints, int elapsedSeconds)
{
    if (setpoints.empty())
        return -1;

    for (size_t row = 0; row < setpoints.size(); ++row)
    {
        if (elapsedSeconds <= static_cast<int>(std::ceil(setpoints[row].time)))
            return static_cast<int>(row);
    }

    return static_cast<int>(setpoints.size()) - 1;
}

double ProfileSetupLogic::displayValueForMode(const Setpoint& setpoint, ProfilePlotWidget::DisplayMode mode)
{
    switch (mode)
    {
        case ProfilePlotWidget::DisplayMode::Current:
            return setpoint.current;
        case ProfilePlotWidget::DisplayMode::Temperature:
            return setpoint.temperature;
        case ProfilePlotWidget::DisplayMode::Voltage:
        case ProfilePlotWidget::DisplayMode::All:
        default:
            return setpoint.voltage;
    }
}

double ProfileSetupLogic::interpolateProfileValue(const std::vector<Setpoint>& setpoints, double timeSeconds,
                                                  ProfilePlotWidget::DisplayMode mode)
{
    if (setpoints.empty())
        return 0.0;

    if (timeSeconds <= setpoints.front().time)
        return displayValueForMode(setpoints.front(), mode);

    for (size_t i = 0; i + 1 < setpoints.size(); ++i)
    {
        const Setpoint& start = setpoints[i];
        const Setpoint& end = setpoints[i + 1];
        if (timeSeconds > end.time)
            continue;

        const double v0 = displayValueForMode(start, mode);
        const double v1 = displayValueForMode(end, mode);
        if (end.time <= start.time)
            return v1;

        const double ratio = std::clamp((timeSeconds - start.time) / (end.time - start.time), 0.0, 1.0);
        if (end.curveType == "Ramp")
        {
            if (end.rampStep <= 0.0)
                return ratio < 1.0 ? v0 : v1;

            const double delta = v1 - v0;
            const double absDelta = std::abs(delta);
            const int steps = std::max(1, static_cast<int>(std::ceil(absDelta / end.rampStep)));
            const double stepDuration = (end.time - start.time) / steps;
            const int stepIndex =
                std::min(steps, static_cast<int>(std::floor((timeSeconds - start.time) / stepDuration)));
            const double value = v0 + (delta >= 0 ? 1.0 : -1.0) * end.rampStep * stepIndex;
            return delta >= 0 ? std::min(value, v1) : std::max(value, v1);
        }

        if (end.curveType == "Exponential")
        {
            if (v0 == 0.0 || v1 == 0.0)
                return v0 + (v1 - v0) * ratio;

            return v0 * std::pow(v1 / v0, ratio);
        }

        return v0 + (v1 - v0) * ratio;
    }

    return displayValueForMode(setpoints.back(), mode);
}

ProfileStepState ProfileSetupLogic::stepStateForElapsedSeconds(const std::vector<Setpoint>& rawSetpoints,
                                                               int elapsedSeconds, int controlModeIndex)
{
    ProfileStepState state;
    if (rawSetpoints.empty())
        return state;

    state.valid = true;
    state.activeRow = activeRowForElapsedSeconds(rawSetpoints, elapsedSeconds);
    state.commandMode = commandModeForControlIndex(controlModeIndex);
    state.markerTime = static_cast<double>(elapsedSeconds);

    const ProfilePlotWidget::DisplayMode displayMode =
        state.commandMode == 1 ? ProfilePlotWidget::DisplayMode::Voltage : ProfilePlotWidget::DisplayMode::Current;
    const std::vector<Setpoint> normalized = normalizedSetpoints(rawSetpoints);
    state.markerValue = interpolateProfileValue(normalized, state.markerTime, displayMode);
    state.setpoint = state.markerValue;
    state.finished = state.activeRow == static_cast<int>(rawSetpoints.size()) - 1 &&
                     elapsedSeconds >= static_cast<int>(std::ceil(rawSetpoints.back().time));
    return state;
}
