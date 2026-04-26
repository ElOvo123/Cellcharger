#pragma once

#include <array>

namespace SystemRequirements
{
struct Range
{
    double min = 0.0;
    double max = 0.0;
};

struct Resolution
{
    double voltageV = 0.001;
    double currentA = 0.1;
    double temperatureDegC = 0.1;
    double pressureBar = 0.001;
};

inline constexpr Range voltageRangeV{0.0, 5.5};
inline constexpr Range currentRangeA{-100.0, 100.0};
inline constexpr Range temperatureRangeDegC{-20.0, 60.0};
inline constexpr Range pressureRangeBar{0.5, 2.0};
inline constexpr Range pulseCurrentRangeA{5.0, 100.0};
inline constexpr Range pulseWidthMs{10.0, 5000.0};
inline constexpr Range internalResistanceMilliOhm{0.1, 100.0};
inline constexpr Resolution resolution{};

inline constexpr double continuousPowerW = 550.0;
inline constexpr double fullLoadEfficiency = 0.90;
inline constexpr double integrationUpdateMs = 100.0;
inline constexpr double standardLoggingHz = 1000.0;
inline constexpr double transientLoggingHz = 5000.0;
inline constexpr double irSamplingHz = 5000.0;
inline constexpr int maxCyclesPerScript = 10000;

inline constexpr int commandModeCC = 0;
inline constexpr int commandModeCV = 1;
inline constexpr int commandModeCP = 2;
inline constexpr int commandModeCR = 3;

inline constexpr std::array<const char*, 71> requirementIds = {
    "ELE-01",  "ELE-02",  "ELE-03",  "ELE-04",  "ELE-05",  "ELE-06",  "ELE-07",  "ELE-08",
    "MEAS-01", "MEAS-02", "MEAS-03", "MEAS-04", "MEAS-05", "MEAS-06", "MEAS-07", "MEAS-08",
    "MEAS-09", "MEAS-10", "MEAS-11", "MEAS-12", "TEST-01", "TEST-02", "TEST-03", "TEST-04",
    "TEST-05", "TEST-06", "TEST-07", "ENV-01",  "ENV-02",  "ENV-03",  "ENV-04",  "ENV-05",
    "ENV-06",  "ENV-07",  "ENV-08",  "ENV-09",  "SAF-01",  "SAF-02",  "SAF-03",  "SAF-04",
    "SAF-05",  "SAF-06",  "SAF-07",  "SAF-08",  "MECH-01", "MECH-02", "MECH-03", "MECH-04",
    "MECH-05", "MECH-06", "MECH-07", "CAL-01",  "CAL-02",  "CAL-03",  "CAL-04",  "CAL-05",
    "CAL-06",  "SW-01",   "SW-02",   "SW-03",   "SW-04",   "SW-05",   "SW-06",   "SW-07",
    "PERF-01", "PERF-02", "PERF-03", "PERF-04", "COMP-01", "COMP-02", "COMP-03"};
} // namespace SystemRequirements
