#include "test_recipe.h"

namespace
{
bool commandModeValid(int mode)
{
    return mode == SystemRequirements::commandModeCC || mode == SystemRequirements::commandModeCV ||
           mode == SystemRequirements::commandModeCP || mode == SystemRequirements::commandModeCR;
}

bool setpointValid(int mode, double setpoint)
{
    if (mode == SystemRequirements::commandModeCV)
        return setpoint >= SystemRequirements::voltageRangeV.min && setpoint <= SystemRequirements::voltageRangeV.max;
    return setpoint >= SystemRequirements::currentRangeA.min && setpoint <= SystemRequirements::currentRangeA.max;
}
} // namespace

bool TestRecipeValidator::validate(const TestRecipe& recipe, QString* errorMessage)
{
    if (recipe.cycles < 1 || recipe.cycles > SystemRequirements::maxCyclesPerScript)
    {
        if (errorMessage)
            *errorMessage = "cycle count is outside SRS limit";
        return false;
    }

    if (recipe.steps.empty())
    {
        if (errorMessage)
            *errorMessage = "recipe must contain at least one step";
        return false;
    }

    for (const TestRecipeStep& step : recipe.steps)
    {
        if (!commandModeValid(step.commandMode))
        {
            if (errorMessage)
                *errorMessage = "recipe step command mode is unsupported";
            return false;
        }

        if (step.type != TestStepType::Rest && !setpointValid(step.commandMode, step.setpoint))
        {
            if (errorMessage)
                *errorMessage = "recipe step setpoint is outside SRS range";
            return false;
        }

        if (step.stop.temperatureEnabled && (step.stop.temperatureDegC < SystemRequirements::temperatureRangeDegC.min ||
                                             step.stop.temperatureDegC > SystemRequirements::temperatureRangeDegC.max))
        {
            if (errorMessage)
                *errorMessage = "temperature condition is outside SRS range";
            return false;
        }

        if (step.stop.pressureEnabled && (step.stop.pressureBar < SystemRequirements::pressureRangeBar.min ||
                                          step.stop.pressureBar > SystemRequirements::pressureRangeBar.max))
        {
            if (errorMessage)
                *errorMessage = "pressure condition is outside SRS range";
            return false;
        }
    }

    return true;
}
