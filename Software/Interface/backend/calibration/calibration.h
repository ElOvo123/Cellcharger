#pragma once

#include <QString>
#include <vector>

struct CalibrationCoefficients
{
    double gain = 1.0;
    double offset = 0.0;
};

struct SelfTestResult
{
    bool passed = true;
    std::vector<QString> failedChecks;
};

class CalibrationModel
{
public:
    static CalibrationCoefficients fromTwoPoints(double rawLow, double referenceLow, double rawHigh,
                                                 double referenceHigh, QString* errorMessage = nullptr);
    static double apply(double rawValue, const CalibrationCoefficients& coefficients);
    static SelfTestResult selfTest(bool adcOk, bool shuntOk, bool sensorOk);
};
