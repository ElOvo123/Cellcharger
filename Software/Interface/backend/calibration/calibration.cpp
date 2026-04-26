#include "calibration.h"

#include <cmath>

CalibrationCoefficients CalibrationModel::fromTwoPoints(double rawLow, double referenceLow, double rawHigh,
                                                        double referenceHigh, QString* errorMessage)
{
    if (std::abs(rawHigh - rawLow) < 1e-12)
    {
        if (errorMessage)
            *errorMessage = "calibration raw points must be distinct";
        return {};
    }

    CalibrationCoefficients coefficients;
    coefficients.gain = (referenceHigh - referenceLow) / (rawHigh - rawLow);
    coefficients.offset = referenceLow - coefficients.gain * rawLow;
    return coefficients;
}

double CalibrationModel::apply(double rawValue, const CalibrationCoefficients& coefficients)
{
    return rawValue * coefficients.gain + coefficients.offset;
}

SelfTestResult CalibrationModel::selfTest(bool adcOk, bool shuntOk, bool sensorOk)
{
    SelfTestResult result;
    if (!adcOk)
        result.failedChecks.push_back("ADC");
    if (!shuntOk)
        result.failedChecks.push_back("shunt");
    if (!sensorOk)
        result.failedChecks.push_back("sensor");

    result.passed = result.failedChecks.empty();
    return result;
}
