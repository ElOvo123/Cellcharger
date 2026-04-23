#pragma once

#include <QWidget>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPoint>
#include <QRect>
#include <QString>
#include <vector>

struct Setpoint
{
    double time = 0.0;
    double voltage = 0.0;
    double current = 0.0;
    double temperature = 0.0;
    QString curveType = "Linear";
    double rampStep = 1.0;
};

class ProfilePlotWidget : public QWidget
{
    Q_OBJECT

public:
    enum class DisplayMode
    {
        All,
        Voltage,
        Current,
        Temperature
    };

    explicit ProfilePlotWidget(QWidget *parent = nullptr);

    void setSetpoints(const std::vector<Setpoint>& setpoints);
    void setDisplayMode(DisplayMode mode);
    void setActiveStepMarker(bool visible, double time, double value);
    void updatePlot();

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    void resetViewRange(double minTime, double maxTime, double minValue, double maxValue);
    double mapToTime(int x, const QRect& plotRect) const;
    double mapToValue(int y, const QRect& plotRect) const;

private:
    std::vector<Setpoint> m_setpoints;
    DisplayMode m_displayMode = DisplayMode::All;
    bool m_useCustomView = false;
    double m_viewMinTime = 0.0;
    double m_viewMaxTime = 1.0;
    double m_viewMinValue = 0.0;
    double m_viewMaxValue = 1.0;
    bool m_panning = false;
    QPoint m_lastPanPos;
    bool m_selecting = false;
    QRect m_selectionRect;
    bool m_activeStepVisible = false;
    double m_activeStepTime = 0.0;
    double m_activeStepValue = 0.0;

    double interpolate(double t0, double v0, double t1, double v1, double t, const QString& curveType, double rampStep);
    static double paddedLowerBound(double minValue, double maxValue);
    static double paddedUpperBound(double minValue, double maxValue);
};
