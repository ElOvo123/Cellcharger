#pragma once

#include "profile_plot_widget.h"

#include <QRect>
#include <QString>
#include <QVector>

#include <optional>
#include <vector>

struct ProfilePlotViewState
{
    double minTime = 0.0;
    double maxTime = 1.0;
    double minValue = 0.0;
    double maxValue = 1.0;
};

struct ProfilePlotBounds
{
    bool hasSetpoints = false;
    bool hasDistinctTimeRange = false;
    std::vector<Setpoint> plotPoints;
    QVector<double> displayValues;
    double minTime = 0.0;
    double maxTime = 0.0;
    double minValue = 0.0;
    double maxValue = 0.0;
};

class ProfilePlotLogic
{
public:
    static QRect plotRectForSize(int width, int height);
    static std::vector<Setpoint> normalizedPlotPoints(const std::vector<Setpoint>& setpoints);
    static double valueForDisplay(const Setpoint& setpoint, ProfilePlotWidget::DisplayMode mode);
    static QString displayLabel(ProfilePlotWidget::DisplayMode mode);
    static ProfilePlotBounds computeBounds(const std::vector<Setpoint>& setpoints,
                                           ProfilePlotWidget::DisplayMode mode);
    static ProfilePlotViewState normalizedViewState(const ProfilePlotViewState& current);
    static ProfilePlotViewState defaultViewState(const ProfilePlotBounds& bounds);
    static std::optional<ProfilePlotViewState> zoomedViewState(const QRect& plotRect,
                                                               const QPointF& position,
                                                               int angleDeltaY,
                                                               const ProfilePlotViewState& current);
    static std::optional<ProfilePlotViewState> selectedViewState(const QRect& plotRect,
                                                                 const QRect& selectionRect,
                                                                 const ProfilePlotViewState& current);
};
