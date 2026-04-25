#include "profile_plot_logic.h"

#include <algorithm>
#include <cmath>

namespace
{
double paddedLowerBound(double minValue, double maxValue)
{
    const double range = maxValue - minValue;
    return minValue - std::max(range * 0.1, 1.0);
}

double paddedUpperBound(double minValue, double maxValue)
{
    const double range = maxValue - minValue;
    return maxValue + std::max(range * 0.1, 1.0);
}

double mapToTime(int x, const QRect& plotRect, const ProfilePlotViewState& view)
{
    return view.minTime + (x - plotRect.left()) * (view.maxTime - view.minTime) / double(plotRect.width());
}

double mapToValue(int y, const QRect& plotRect, const ProfilePlotViewState& view)
{
    return view.maxValue - (y - plotRect.top()) * (view.maxValue - view.minValue) / double(plotRect.height());
}
} // namespace

QRect ProfilePlotLogic::plotRectForSize(int width, int height)
{
    return QRect(70, 20, width - 70 - 20, height - 20 - 60);
}

std::vector<Setpoint> ProfilePlotLogic::normalizedPlotPoints(const std::vector<Setpoint>& setpoints)
{
    std::vector<Setpoint> plotPoints = setpoints;
    std::sort(plotPoints.begin(), plotPoints.end(),
              [](const Setpoint& lhs, const Setpoint& rhs) { return lhs.time < rhs.time; });

    if (!plotPoints.empty() && plotPoints.front().time > 0.0)
    {
        Setpoint baseline;
        baseline.time = 0.0;
        baseline.current = 0.0;
        baseline.voltage = plotPoints.front().voltage;
        baseline.temperature = 25.0;
        baseline.curveType = plotPoints.front().curveType;
        baseline.rampStep = plotPoints.front().rampStep;
        plotPoints.insert(plotPoints.begin(), baseline);
    }

    return plotPoints;
}

double ProfilePlotLogic::valueForDisplay(const Setpoint& setpoint, ProfilePlotWidget::DisplayMode mode)
{
    switch (mode)
    {
        case ProfilePlotWidget::DisplayMode::Voltage:
            return setpoint.voltage;
        case ProfilePlotWidget::DisplayMode::Current:
            return setpoint.current;
        case ProfilePlotWidget::DisplayMode::Temperature:
            return setpoint.temperature;
        case ProfilePlotWidget::DisplayMode::All:
        default:
            return setpoint.voltage;
    }
}

QString ProfilePlotLogic::displayLabel(ProfilePlotWidget::DisplayMode mode)
{
    switch (mode)
    {
        case ProfilePlotWidget::DisplayMode::Voltage:
            return "Voltage (V)";
        case ProfilePlotWidget::DisplayMode::Current:
            return "Current (A)";
        case ProfilePlotWidget::DisplayMode::Temperature:
            return "Temperature (°C)";
        case ProfilePlotWidget::DisplayMode::All:
        default:
            return "Value";
    }
}

ProfilePlotBounds ProfilePlotLogic::computeBounds(const std::vector<Setpoint>& setpoints,
                                                  ProfilePlotWidget::DisplayMode mode)
{
    ProfilePlotBounds bounds;
    bounds.hasSetpoints = !setpoints.empty();
    if (!bounds.hasSetpoints)
        return bounds;

    bounds.plotPoints = normalizedPlotPoints(setpoints);
    bounds.minTime = bounds.plotPoints.front().time;
    bounds.maxTime = bounds.plotPoints.front().time;
    for (const Setpoint& setpoint : bounds.plotPoints)
    {
        bounds.minTime = std::min(bounds.minTime, setpoint.time);
        bounds.maxTime = std::max(bounds.maxTime, setpoint.time);
    }

    bounds.hasDistinctTimeRange = bounds.maxTime > bounds.minTime;
    if (!bounds.hasDistinctTimeRange)
        return bounds;

    if (mode == ProfilePlotWidget::DisplayMode::All)
    {
        for (const Setpoint& setpoint : bounds.plotPoints)
        {
            bounds.displayValues.push_back(setpoint.voltage);
            bounds.displayValues.push_back(setpoint.current);
            bounds.displayValues.push_back(setpoint.temperature);
        }
    }
    else
    {
        bounds.displayValues.reserve(static_cast<qsizetype>(bounds.plotPoints.size()));
        for (const Setpoint& setpoint : bounds.plotPoints)
            bounds.displayValues.push_back(valueForDisplay(setpoint, mode));
    }

    bounds.minValue = *std::min_element(bounds.displayValues.begin(), bounds.displayValues.end());
    bounds.maxValue = *std::max_element(bounds.displayValues.begin(), bounds.displayValues.end());
    if (bounds.maxValue <= bounds.minValue)
    {
        bounds.maxValue = bounds.minValue + 1.0;
        bounds.minValue -= 1.0;
    }

    bounds.minValue = paddedLowerBound(bounds.minValue, bounds.maxValue);
    bounds.maxValue = paddedUpperBound(bounds.minValue, bounds.maxValue);
    return bounds;
}

ProfilePlotViewState ProfilePlotLogic::normalizedViewState(const ProfilePlotViewState& current)
{
    ProfilePlotViewState normalized = current;
    if (normalized.maxTime <= normalized.minTime)
        normalized.maxTime = normalized.minTime + 1.0;
    if (normalized.maxValue <= normalized.minValue)
        normalized.maxValue = normalized.minValue + 1.0;
    return normalized;
}

ProfilePlotViewState ProfilePlotLogic::defaultViewState(const ProfilePlotBounds& bounds)
{
    return normalizedViewState({bounds.minTime, bounds.maxTime, bounds.minValue, bounds.maxValue});
}

std::optional<ProfilePlotViewState> ProfilePlotLogic::zoomedViewState(const QRect& plotRect, const QPointF& position,
                                                                      int angleDeltaY,
                                                                      const ProfilePlotViewState& current)
{
    if (!plotRect.contains(position.toPoint()))
        return std::nullopt;

    const ProfilePlotViewState view = normalizedViewState(current);
    const double zoomFactor = std::pow(1.1, angleDeltaY / 120.0);
    const double centerTime = mapToTime(static_cast<int>(position.x()), plotRect, view);
    const double centerValue = mapToValue(static_cast<int>(position.y()), plotRect, view);

    ProfilePlotViewState zoomed = view;
    const double timeRange = (view.maxTime - view.minTime) / zoomFactor;
    const double valueRange = (view.maxValue - view.minValue) / zoomFactor;

    zoomed.minTime = centerTime - (centerTime - view.minTime) / zoomFactor;
    zoomed.maxTime = zoomed.minTime + timeRange;
    zoomed.minValue = centerValue - (centerValue - view.minValue) / zoomFactor;
    zoomed.maxValue = zoomed.minValue + valueRange;
    return normalizedViewState(zoomed);
}

std::optional<ProfilePlotViewState> ProfilePlotLogic::selectedViewState(const QRect& plotRect,
                                                                        const QRect& selectionRect,
                                                                        const ProfilePlotViewState& current)
{
    QRect normalizedSelection = selectionRect.normalized();
    if (normalizedSelection.width() <= 10 || normalizedSelection.height() <= 10)
        return std::nullopt;
    if (!normalizedSelection.intersects(plotRect))
        return std::nullopt;

    const QRect clipped = normalizedSelection.intersected(plotRect);
    const ProfilePlotViewState view = normalizedViewState(current);
    ProfilePlotViewState selected;
    selected.minTime = mapToTime(clipped.left(), plotRect, view);
    selected.maxTime = mapToTime(clipped.right(), plotRect, view);
    selected.maxValue = mapToValue(clipped.top(), plotRect, view);
    selected.minValue = mapToValue(clipped.bottom(), plotRect, view);

    if (selected.maxTime <= selected.minTime || selected.maxValue <= selected.minValue)
        return std::nullopt;

    return normalizedViewState(selected);
}
