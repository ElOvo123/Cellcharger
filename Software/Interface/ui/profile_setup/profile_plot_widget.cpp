#include "profile_plot_widget.h"

#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QMouseEvent>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

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

    const QRect plotRect(70, 20, width() - 70 - 20, height() - 20 - 60);
    if (!plotRect.contains(event->position().toPoint())) {
        return;
    }

    const double zoomFactor = std::pow(1.1, event->angleDelta().y() / 120.0);
    double centerTime = mapToTime(event->position().x(), plotRect);
    double centerValue = mapToValue(event->position().y(), plotRect);

    double timeRange = (m_viewMaxTime - m_viewMinTime) / zoomFactor;
    double valueRange = (m_viewMaxValue - m_viewMinValue) / zoomFactor;

    m_viewMinTime = centerTime - (centerTime - m_viewMinTime) / zoomFactor;
    m_viewMaxTime = m_viewMinTime + timeRange;
    m_viewMinValue = centerValue - (centerValue - m_viewMinValue) / zoomFactor;
    m_viewMaxValue = m_viewMinValue + valueRange;
    m_useCustomView = true;
    update();
}

void ProfilePlotWidget::mousePressEvent(QMouseEvent *event)
{
    const QRect plotRect(70, 20, width() - 70 - 20, height() - 20 - 60);
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
    const QRect plotRect(70, 20, width() - 70 - 20, height() - 20 - 60);
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
    const QRect plotRect(70, 20, width() - 70 - 20, height() - 20 - 60);
    if (m_panning && event->button() == Qt::MiddleButton) {
        m_panning = false;
        unsetCursor();
    } else if (m_selecting && event->button() == Qt::LeftButton) {
        m_selecting = false;
        QRect normalized = m_selectionRect.normalized();
        if (normalized.width() > 10 && normalized.height() > 10 && normalized.intersects(plotRect)) {
            QRect clipped = normalized.intersected(plotRect);
            double newMinTime = mapToTime(clipped.left(), plotRect);
            double newMaxTime = mapToTime(clipped.right(), plotRect);
            double newMaxValue = mapToValue(clipped.top(), plotRect);
            double newMinValue = mapToValue(clipped.bottom(), plotRect);
            if (newMaxTime > newMinTime && newMaxValue > newMinValue) {
                m_viewMinTime = newMinTime;
                m_viewMaxTime = newMaxTime;
                m_viewMinValue = newMinValue;
                m_viewMaxValue = newMaxValue;
                m_useCustomView = true;
            }
        }
        update();
    }
}

namespace {
static double valueForDisplay(const Setpoint& sp, ProfilePlotWidget::DisplayMode mode)
{
    switch (mode) {
        case ProfilePlotWidget::DisplayMode::Voltage:
            return sp.voltage;
        case ProfilePlotWidget::DisplayMode::Current:
            return sp.current;
        case ProfilePlotWidget::DisplayMode::Temperature:
            return sp.temperature;
        case ProfilePlotWidget::DisplayMode::All:
        default:
            return sp.voltage;
    }
}

static QString displayLabel(ProfilePlotWidget::DisplayMode mode)
{
    switch (mode) {
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

    std::vector<Setpoint> plotPoints = m_setpoints;
    std::sort(plotPoints.begin(), plotPoints.end(), [](const Setpoint &a, const Setpoint &b) {
        return a.time < b.time;
    });

    if (plotPoints.front().time > 0.0) {
        Setpoint baseline;
        baseline.time = 0.0;
        baseline.current = 0.0;
        baseline.voltage = plotPoints.front().voltage;
        baseline.temperature = 25.0;
        baseline.curveType = plotPoints.front().curveType;
        baseline.rampStep = plotPoints.front().rampStep;
        plotPoints.insert(plotPoints.begin(), baseline);
    }

    double minTime = plotPoints.front().time;
    double maxTime = plotPoints.front().time;
    for (const auto& sp : plotPoints) {
        minTime = std::min(minTime, sp.time);
        maxTime = std::max(maxTime, sp.time);
    }

    if (maxTime <= minTime) {
        painter.drawText(rect(), Qt::AlignCenter, "Set distinct time values for each setpoint.");
        return;
    }

    const int leftMargin = 70;
    const int rightMargin = 20;
    const int topMargin = 20;
    const int bottomMargin = 60;
    QRect plotRect(leftMargin, topMargin, width() - leftMargin - rightMargin, height() - topMargin - bottomMargin);

    QVector<double> displayValues;

    if (m_displayMode == DisplayMode::All) {
        for (const auto& sp : plotPoints) {
            displayValues.push_back(sp.voltage);
            displayValues.push_back(sp.current);
            displayValues.push_back(sp.temperature);
        }
    } else {
        displayValues.reserve(plotPoints.size());
        for (const auto& sp : plotPoints)
            displayValues.push_back(valueForDisplay(sp, m_displayMode));
    }

    double minValue = *std::min_element(displayValues.begin(), displayValues.end());
    double maxValue = *std::max_element(displayValues.begin(), displayValues.end());

    if (maxValue <= minValue) {
        maxValue = minValue + 1.0;
        minValue = minValue - 1.0;
    }

    minValue = paddedLowerBound(minValue, maxValue);
    maxValue = paddedUpperBound(minValue, maxValue);

    if (!m_useCustomView) {
        m_viewMinTime = minTime;
        m_viewMaxTime = maxTime;
        m_viewMinValue = minValue;
        m_viewMaxValue = maxValue;
    }

    double viewMinTime = m_viewMinTime;
    double viewMaxTime = m_viewMaxTime;
    double viewMinValue = m_viewMinValue;
    double viewMaxValue = m_viewMaxValue;

    if (viewMaxTime <= viewMinTime) {
        viewMaxTime = viewMinTime + 1.0;
    }
    if (viewMaxValue <= viewMinValue) {
        viewMaxValue = viewMinValue + 1.0;
    }

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
    painter.drawText(0, 0, displayLabel(m_displayMode));
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
            double value0 = valueForDisplay(sp0, mode);
            double value1 = valueForDisplay(sp1, mode);
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
        double y = plotRect.bottom() - (valueForDisplay(sp, m_displayMode) - viewMinValue) / (viewMaxValue - viewMinValue) * plotRect.height();
        painter.drawEllipse(QPointF(x, y), 4, 4);
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