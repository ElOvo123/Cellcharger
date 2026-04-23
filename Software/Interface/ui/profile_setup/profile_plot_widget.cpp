#include "profile_plot_widget.h"
#include "profile_plot_logic.h"

#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QMouseEvent>
#include <QWheelEvent>

ProfilePlotWidget::ProfilePlotWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(400, 200);
}

void ProfilePlotWidget::setSetpoints(const std::vector<Setpoint>& setpoints)
{
    m_setpoints = setpoints;
    m_useCustomView = false;
    updatePlot();
}

void ProfilePlotWidget::setDisplayMode(DisplayMode mode)
{
    m_displayMode = mode;
    updatePlot();
}

void ProfilePlotWidget::setActiveStepMarker(bool visible, double time, double value)
{
    m_activeStepVisible = visible;
    m_activeStepTime = time;
    m_activeStepValue = value;
    updatePlot();
}

void ProfilePlotWidget::updatePlot()
{
    update();
}

void ProfilePlotWidget::resetViewRange(double minTime, double maxTime, double minValue, double maxValue)
{
    m_viewMinTime = minTime;
    m_viewMaxTime = maxTime;
    m_viewMinValue = minValue;
    m_viewMaxValue = maxValue;
    m_useCustomView = false;
}

double ProfilePlotWidget::mapToTime(int x, const QRect& plotRect) const
{
    return m_viewMinTime + (x - plotRect.left()) * (m_viewMaxTime - m_viewMinTime) / double(plotRect.width());
}

double ProfilePlotWidget::mapToValue(int y, const QRect& plotRect) const
{
    return m_viewMaxValue - (y - plotRect.top()) * (m_viewMaxValue - m_viewMinValue) / double(plotRect.height());
}

void ProfilePlotWidget::wheelEvent(QWheelEvent *event)
{
    if (m_setpoints.empty()) {
        return;
    }

    const QRect plotRect = ProfilePlotLogic::plotRectForSize(width(), height());
    const std::optional<ProfilePlotViewState> zoomed =
        ProfilePlotLogic::zoomedViewState(
            plotRect,
            event->position(),
            event->angleDelta().y(),
            {m_viewMinTime, m_viewMaxTime, m_viewMinValue, m_viewMaxValue});
    if (!zoomed.has_value()) {
        return;
    }

    m_viewMinTime = zoomed->minTime;
    m_viewMaxTime = zoomed->maxTime;
    m_viewMinValue = zoomed->minValue;
    m_viewMaxValue = zoomed->maxValue;
    m_useCustomView = true;
    update();
}

void ProfilePlotWidget::mousePressEvent(QMouseEvent *event)
{
    const QRect plotRect = ProfilePlotLogic::plotRectForSize(width(), height());
    if (!plotRect.contains(event->pos())) {
        return;
    }

    if (event->button() == Qt::RightButton) {
        m_useCustomView = false;
        update();
        return;
    }

    if (event->button() == Qt::MiddleButton) {
        m_panning = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    } else if (event->button() == Qt::LeftButton) {
        m_selecting = true;
        m_selectionRect = QRect(event->pos(), QSize(0, 0));
    }
}

void ProfilePlotWidget::mouseMoveEvent(QMouseEvent *event)
{
    const QRect plotRect = ProfilePlotLogic::plotRectForSize(width(), height());
    if (m_panning) {
        QPoint delta = event->pos() - m_lastPanPos;
        double timeDelta = -delta.x() * (m_viewMaxTime - m_viewMinTime) / double(plotRect.width());
        double valueDelta = delta.y() * (m_viewMaxValue - m_viewMinValue) / double(plotRect.height());
        m_viewMinTime += timeDelta;
        m_viewMaxTime += timeDelta;
        m_viewMinValue += valueDelta;
        m_viewMaxValue += valueDelta;
        m_lastPanPos = event->pos();
        m_useCustomView = true;
        update();
    } else if (m_selecting) {
        m_selectionRect.setBottomRight(event->pos());
        update();
    }
}

void ProfilePlotWidget::mouseReleaseEvent(QMouseEvent *event)
{
    const QRect plotRect = ProfilePlotLogic::plotRectForSize(width(), height());
    if (m_panning && event->button() == Qt::MiddleButton) {
        m_panning = false;
        unsetCursor();
    } else if (m_selecting && event->button() == Qt::LeftButton) {
        m_selecting = false;
        const std::optional<ProfilePlotViewState> selected =
            ProfilePlotLogic::selectedViewState(
                plotRect,
                m_selectionRect,
                {m_viewMinTime, m_viewMaxTime, m_viewMinValue, m_viewMaxValue});
        if (selected.has_value()) {
            m_viewMinTime = selected->minTime;
            m_viewMaxTime = selected->maxTime;
            m_viewMinValue = selected->minValue;
            m_viewMaxValue = selected->maxValue;
            m_useCustomView = true;
        }
        update();
    }
}

void ProfilePlotWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);

    if (m_setpoints.empty()) {
        painter.drawText(rect(), Qt::AlignCenter, "Add a setpoint to see the profile");
        return;
    }

    const ProfilePlotBounds bounds = ProfilePlotLogic::computeBounds(m_setpoints, m_displayMode);
    if (!bounds.hasDistinctTimeRange) {
        painter.drawText(rect(), Qt::AlignCenter, "Set distinct time values for each setpoint.");
        return;
    }
    const std::vector<Setpoint>& plotPoints = bounds.plotPoints;

    const QRect plotRect = ProfilePlotLogic::plotRectForSize(width(), height());

    if (!m_useCustomView) {
        const ProfilePlotViewState defaultView = ProfilePlotLogic::defaultViewState(bounds);
        m_viewMinTime = defaultView.minTime;
        m_viewMaxTime = defaultView.maxTime;
        m_viewMinValue = defaultView.minValue;
        m_viewMaxValue = defaultView.maxValue;
    }

    const ProfilePlotViewState viewState =
        ProfilePlotLogic::normalizedViewState({m_viewMinTime, m_viewMaxTime, m_viewMinValue, m_viewMaxValue});
    const double viewMinTime = viewState.minTime;
    const double viewMaxTime = viewState.maxTime;
    const double viewMinValue = viewState.minValue;
    const double viewMaxValue = viewState.maxValue;

    painter.setPen(QPen(Qt::black, 2));
    painter.drawLine(plotRect.left(), plotRect.bottom(), plotRect.right(), plotRect.bottom());
    painter.drawLine(plotRect.left(), plotRect.bottom(), plotRect.left(), plotRect.top());

    painter.setPen(QPen(Qt::lightGray, 1, Qt::DotLine));
    for (int i = 0; i <= 5; ++i) {
        int x = plotRect.left() + i * plotRect.width() / 5;
        painter.drawLine(x, plotRect.top(), x, plotRect.bottom());
        double timeValue = viewMinTime + (viewMaxTime - viewMinTime) * i / 5.0;
        painter.setPen(Qt::black);
        painter.drawText(x - 15, plotRect.bottom() + 18, QString::number(timeValue, 'f', 0));
        painter.setPen(QPen(Qt::lightGray, 1, Qt::DotLine));
    }
    for (int i = 0; i <= 5; ++i) {
        int y = plotRect.bottom() - i * plotRect.height() / 5;
        painter.drawLine(plotRect.left(), y, plotRect.right(), y);
        double value = viewMinValue + (viewMaxValue - viewMinValue) * i / 5.0;
        painter.setPen(Qt::black);
        painter.drawText(plotRect.left() - 45, y + 4, QString::number(value, 'f', 1));
        painter.setPen(QPen(Qt::lightGray, 1, Qt::DotLine));
    }

    painter.setPen(QPen(Qt::black, 2));
    painter.drawText(plotRect.right() - 40, plotRect.bottom() + 35, "Time (s)");
    painter.save();
    painter.translate(plotRect.left() - 55, plotRect.center().y());
    painter.rotate(-90);
    painter.drawText(0, 0, ProfilePlotLogic::displayLabel(m_displayMode));
    painter.restore();

    painter.save();
    painter.setClipRect(plotRect);
    painter.setRenderHint(QPainter::Antialiasing);

    auto drawCurve = [&](ProfilePlotWidget::DisplayMode mode, const QColor& color) {
        painter.setPen(QPen(color, 2));
        QPointF prevPoint;
        bool firstPoint = true;
        for (size_t i = 0; i < plotPoints.size() - 1; ++i) {
            const auto& sp0 = plotPoints[i];
            const auto& sp1 = plotPoints[i + 1];
            double value0 = ProfilePlotLogic::valueForDisplay(sp0, mode);
            double value1 = ProfilePlotLogic::valueForDisplay(sp1, mode);
            for (int step = 0; step <= 50; ++step) {
                double t = sp0.time + (sp1.time - sp0.time) * step / 50.0;
                double value = interpolate(sp0.time, value0, sp1.time, value1, t, sp1.curveType, sp1.rampStep);
                double x = plotRect.left() + (t - viewMinTime) / (viewMaxTime - viewMinTime) * plotRect.width();
                double y = plotRect.bottom() - (value - viewMinValue) / (viewMaxValue - viewMinValue) * plotRect.height();
                if (firstPoint && step == 0) {
                    prevPoint = QPointF(x, y);
                    firstPoint = false;
                } else {
                    QPointF point(x, y);
                    painter.drawLine(prevPoint, point);
                    prevPoint = point;
                }
            }
        }
    };

    if (m_displayMode == DisplayMode::All) {
        drawCurve(DisplayMode::Voltage, Qt::blue);
        drawCurve(DisplayMode::Current, Qt::green);
        drawCurve(DisplayMode::Temperature, Qt::red);
    } else {
        QColor color = Qt::blue;
        if (m_displayMode == DisplayMode::Current) color = Qt::green;
        else if (m_displayMode == DisplayMode::Temperature) color = Qt::red;
        drawCurve(m_displayMode, color);
    }

    painter.setPen(QPen(Qt::black, 1));
    painter.setBrush(Qt::red);
    for (const auto& sp : plotPoints) {
        double x = plotRect.left() + (sp.time - viewMinTime) / (viewMaxTime - viewMinTime) * plotRect.width();
        double y = plotRect.bottom() - (ProfilePlotLogic::valueForDisplay(sp, m_displayMode) - viewMinValue) / (viewMaxValue - viewMinValue) * plotRect.height();
        painter.drawEllipse(QPointF(x, y), 4, 4);
    }

    if (m_activeStepVisible) {
        const double x = plotRect.left() + (m_activeStepTime - viewMinTime) / (viewMaxTime - viewMinTime) * plotRect.width();
        const double y = plotRect.bottom() - (m_activeStepValue - viewMinValue) / (viewMaxValue - viewMinValue) * plotRect.height();
        painter.setPen(QPen(QColor("#1d4ed8"), 2));
        painter.setBrush(QColor("#1d4ed8"));
        painter.drawEllipse(QPointF(x, y), 6, 6);
    }
    painter.restore();

    if (m_selecting) {
        painter.setPen(QPen(Qt::blue, 1, Qt::DashLine));
        painter.setBrush(QColor(0, 128, 255, 50));
        painter.drawRect(m_selectionRect.normalized());
    }

    struct LegendItem { QString label; QColor color; };
    std::vector<LegendItem> legendItems;
    if (m_displayMode == DisplayMode::All) {
        legendItems.push_back({"Voltage", Qt::blue});
        legendItems.push_back({"Current", Qt::green});
        legendItems.push_back({"Temperature", Qt::red});
    } else if (m_displayMode == DisplayMode::Voltage) {
        legendItems.push_back({"Voltage", Qt::blue});
    } else if (m_displayMode == DisplayMode::Current) {
        legendItems.push_back({"Current", Qt::green});
    } else if (m_displayMode == DisplayMode::Temperature) {
        legendItems.push_back({"Temperature", Qt::red});
    }

    const int legendMargin = 10;
    const int legendWidth = 120;
    const int legendHeight = 18 * legendItems.size() + 10;
    QRect legendRect(plotRect.right() - legendWidth - legendMargin, plotRect.top() + legendMargin, legendWidth, legendHeight);
    painter.setPen(QPen(Qt::black, 1));
    painter.setBrush(QColor(255, 255, 255, 230));
    painter.drawRect(legendRect);
    painter.setBrush(Qt::NoBrush);

    int legendTextY = legendRect.top() + 18;
    for (const auto& item : legendItems) {
        painter.setPen(QPen(item.color, 2));
        painter.drawLine(legendRect.left() + 8, legendTextY - 6, legendRect.left() + 28, legendTextY - 6);
        painter.setPen(Qt::black);
        painter.drawText(legendRect.left() + 32, legendTextY, item.label);
        legendTextY += 18;
    }

    painter.setRenderHint(QPainter::Antialiasing);
    if (m_selecting) {
        painter.setPen(QPen(Qt::blue, 1, Qt::DashLine));
        painter.setBrush(QColor(0, 128, 255, 50));
        painter.drawRect(m_selectionRect.normalized());
    }
}

double ProfilePlotWidget::interpolate(double t0, double v0, double t1, double v1, double t, const QString& curveType, double rampStep)
{
    if (t1 == t0) return v0;
    double ratio = std::clamp((t - t0) / (t1 - t0), 0.0, 1.0);
    if (curveType == "Ramp") {
        if (rampStep <= 0.0) {
            return (ratio < 1.0) ? v0 : v1;
        }
        double delta = v1 - v0;
        double absDelta = std::abs(delta);
        int steps = std::max(1, static_cast<int>(std::ceil(absDelta / rampStep)));
        double stepDuration = (t1 - t0) / steps;
        int stepIndex = std::min(steps, static_cast<int>(std::floor((t - t0) / stepDuration)));
        double value = v0 + (delta >= 0 ? 1 : -1) * rampStep * stepIndex;
        if (delta >= 0) {
            return std::min(value, v1);
        }
        return std::max(value, v1);
    } else if (curveType == "Linear") {
        return v0 + (v1 - v0) * ratio;
    } else if (curveType == "Exponential") {
        if (v0 == 0 || v1 == 0) return v0 + (v1 - v0) * ratio;
        double expRatio = std::pow(v1 / v0, ratio);
        return v0 * expRatio;
    }
    return v0 + (v1 - v0) * ratio;
}

double ProfilePlotWidget::paddedLowerBound(double minValue, double maxValue)
{
    double range = maxValue - minValue;
    return minValue - std::max(range * 0.1, 1.0);
}

double ProfilePlotWidget::paddedUpperBound(double minValue, double maxValue)
{
    double range = maxValue - minValue;
    return maxValue + std::max(range * 0.1, 1.0);
}
