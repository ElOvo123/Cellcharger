#pragma once

#include "system_requirements.h"

#include <QString>
#include <vector>

enum class TestStepType
{
    Charge,
    Discharge,
    Rest
};

struct StopConditions
{
    bool voltageEnabled = false;
    double voltageV = 0.0;
    bool currentEnabled = false;
    double currentA = 0.0;
    bool temperatureEnabled = false;
    double temperatureDegC = 0.0;
    bool pressureEnabled = false;
    double pressureBar = 0.0;
    bool timeEnabled = false;
    double timeSeconds = 0.0;
};

struct TestRecipeStep
{
    TestStepType type = TestStepType::Rest;
    int commandMode = SystemRequirements::commandModeCC;
    double setpoint = 0.0;
    StopConditions stop;
};

struct TestRecipe
{
    QString name;
    int cycles = 1;
    std::vector<TestRecipeStep> steps;
};

class TestRecipeValidator
{
public:
    static bool validate(const TestRecipe& recipe, QString* errorMessage = nullptr);
};
