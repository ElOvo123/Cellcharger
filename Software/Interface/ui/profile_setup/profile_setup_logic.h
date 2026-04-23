#pragma once

#include "profile_plot_widget.h"

#include <vector>

struct ProfileStepState
{
    bool valid = false;
    bool finished = false;
    int activeRow = -1;
    double markerTime = 0.0;
    double markerValue = 0.0;
    int commandMode = 0;
    double setpoint = 0.0;
};

class ProfileSetupLogic
{
public:
    static ProfilePlotWidget::DisplayMode displayModeForControlIndex(int controlModeIndex);
    static int commandModeForControlIndex(int controlModeIndex);
    static std::vector<Setpoint> normalizedSetpoints(const std::vector<Setpoint>& setpoints);
    static int activeRowForElapsedSeconds(const std::vector<Setpoint>& setpoints, int elapsedSeconds);
    static double displayValueForMode(const Setpoint& setpoint, ProfilePlotWidget::DisplayMode mode);
    static double interpolateProfileValue(const std::vector<Setpoint>& setpoints,
                                          double timeSeconds,
                                          ProfilePlotWidget::DisplayMode mode);
    static ProfileStepState stepStateForElapsedSeconds(const std::vector<Setpoint>& rawSetpoints,
                                                       int elapsedSeconds,
                                                       int controlModeIndex);
};
