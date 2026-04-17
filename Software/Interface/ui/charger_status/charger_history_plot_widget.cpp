#include "charger_history_plot_widget.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPen>

#include <algorithm>
#include <cmath>

namespace
{
constexpr int kLeftMargin = 54;
constexpr int kRightMargin = 54;
constexpr int kTopMargin = 48;
constexpr int kBottomMargin = 34;
constexpr int kGridLines = 4;
const QColor kVoltageColor("#2f6fed");
const QColor kCurrentColor("#cf3e3e");
const QColor kGridColor("#dfd6a1");
const QColor kAxisColor("#6f6232");
const QColor kBackgroundColor("#ffffff");
const QColor kAxisFillColor("#fff3c2");

QString valueText(double value)
{
    return QString::number(value, 'f', 2);
}
}

ChargerHistoryPlotWidget::ChargerHistoryPlotWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(340);
    setAutoFillBackground(true);
}

void ChargerHistoryPlotWidget::clearHistory()
{
    for (auto& samples : m_history)
        samples.clear();

    update();
}

void ChargerHistoryPlotWidget::appendSample(uint32_t chargerId, double timeSeconds, double voltage, double current)
{
    if (chargerId < 1 || chargerId > 3)
        return;

    auto& samples = m_history[static_cast<size_t>(chargerId - 1)];
    samples.push_back({timeSeconds, voltage, current});
    if (static_cast<int>(samples.size()) > kMaxSamples)
        samples.erase(samples.begin(), samples.begin() + (samples.size() - kMaxSamples));

    if (chargerId == m_selectedCharger)
        update();
}

void ChargerHistoryPlotWidget::setSelectedCharger(uint32_t chargerId)
{
    if (chargerId < 1 || chargerId > 3 || m_selectedCharger == chargerId)
        return;

    m_selectedCharger = chargerId;
    update();
}

uint32_t ChargerHistoryPlotWidget::selectedCharger() const
{
    return m_selectedCharger;
}

int ChargerHistoryPlotWidget::sampleCountForCharger(uint32_t chargerId) const
{
    if (chargerId < 1 || chargerId > 3)
        return 0;

    return static_cast<int>(m_history[static_cast<size_t>(chargerId - 1)].size());
}

void ChargerHistoryPlotWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), kBackgroundColor);

    const QRect plotRect = rect().adjusted(kLeftMargin, kTopMargin, -kRightMargin, -kBottomMargin);
    if (plotRect.width() < 40 || plotRect.height() < 40)
        return;

    painter.fillRect(plotRect.adjusted(-1, -1, 1, 1), kAxisFillColor);

    painter.setPen(QPen(kGridColor, 1));
    for (int i = 0; i <= kGridLines; ++i)
    {
        const int y = plotRect.top() + (plotRect.height() * i) / kGridLines;
        painter.drawLine(plotRect.left(), y, plotRect.right(), y);
    }

    painter.setPen(QPen(kAxisColor, 1));
    painter.drawRect(plotRect);

    const auto& samples = samplesForCharger(m_selectedCharger);
    if (samples.empty())
    {
        painter.setPen(kAxisColor);
        painter.drawText(QRect(plotRect.left(), 20, plotRect.width() / 2, 22),
                         Qt::AlignLeft | Qt::AlignVCenter, "Voltage (V)");
        painter.drawText(QRect(plotRect.center().x(), 20, plotRect.width() / 2, 22),
                         Qt::AlignRight | Qt::AlignVCenter, "Current (A)");
        painter.drawText(plotRect, Qt::AlignCenter,
                         QString("Waiting for Charger %1 data").arg(m_selectedCharger));
        painter.drawText(QRect(plotRect.left(), plotRect.bottom() + 8, plotRect.width(), 20),
                         Qt::AlignCenter, "Time (s)");
        painter.drawText(QRect(2, plotRect.top(), kLeftMargin - 8, 20), Qt::AlignLeft, "V");
        painter.drawText(QRect(plotRect.right() + 8, plotRect.top(), kRightMargin - 10, 20), Qt::AlignRight, "A");
        return;
    }

    double minTime = samples.front().timeSeconds;
    double maxTime = samples.back().timeSeconds;
    if (qFuzzyCompare(minTime + 1.0, maxTime + 1.0))
        maxTime = minTime + 1.0;

    double minVoltage = samples.front().voltage;
    double maxVoltage = samples.front().voltage;
    double minCurrent = samples.front().current;
    double maxCurrent = samples.front().current;
    for (const Sample& sample : samples)
    {
        minVoltage = std::min(minVoltage, sample.voltage);
        maxVoltage = std::max(maxVoltage, sample.voltage);
        minCurrent = std::min(minCurrent, sample.current);
        maxCurrent = std::max(maxCurrent, sample.current);
    }

    const double rawMinVoltage = minVoltage;
    const double rawMaxVoltage = maxVoltage;
    const double rawMinCurrent = minCurrent;
    const double rawMaxCurrent = maxCurrent;
    minVoltage = paddedLowerBound(rawMinVoltage, rawMaxVoltage);
    maxVoltage = paddedUpperBound(rawMinVoltage, rawMaxVoltage);
    minCurrent = paddedLowerBound(rawMinCurrent, rawMaxCurrent);
    maxCurrent = paddedUpperBound(rawMinCurrent, rawMaxCurrent);

    auto xForTime = [&](double timeSeconds)
    {
        return plotRect.left() + ((timeSeconds - minTime) / (maxTime - minTime)) * plotRect.width();
    };
    auto yForVoltage = [&](double voltage)
    {
        return plotRect.bottom() - ((voltage - minVoltage) / (maxVoltage - minVoltage)) * plotRect.height();
    };
    auto yForCurrent = [&](double current)
    {
        return plotRect.bottom() - ((current - minCurrent) / (maxCurrent - minCurrent)) * plotRect.height();
    };

    QPainterPath voltagePath;
    QPainterPath currentPath;
    for (size_t i = 0; i < samples.size(); ++i)
    {
        const QPointF voltagePoint(xForTime(samples[i].timeSeconds), yForVoltage(samples[i].voltage));
        const QPointF currentPoint(xForTime(samples[i].timeSeconds), yForCurrent(samples[i].current));
        if (i == 0)
        {
            voltagePath.moveTo(voltagePoint);
            currentPath.moveTo(currentPoint);
        }
        else
        {
            voltagePath.lineTo(voltagePoint);
            currentPath.lineTo(currentPoint);
        }
    }

    painter.setPen(QPen(kVoltageColor, 2));
    painter.drawPath(voltagePath);
    painter.setPen(QPen(kCurrentColor, 2));
    painter.drawPath(currentPath);

    painter.setPen(QPen(kVoltageColor, 1));
    painter.drawText(QRect(plotRect.left(), 20, plotRect.width() / 2, 22),
                     Qt::AlignLeft | Qt::AlignVCenter, "Voltage (V)");
    painter.setPen(QPen(kCurrentColor, 1));
    painter.drawText(QRect(plotRect.center().x(), 20, plotRect.width() / 2, 22),
                     Qt::AlignRight | Qt::AlignVCenter, "Current (A)");

    painter.setPen(kVoltageColor);
    painter.drawText(QRect(2, plotRect.top() + 8, kLeftMargin - 8, 20),
                     Qt::AlignLeft | Qt::AlignTop,
                     valueText(maxVoltage));
    painter.drawText(QRect(2, plotRect.top() + (plotRect.height() / 2) - 10, kLeftMargin - 8, 20),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     valueText((minVoltage + maxVoltage) / 2.0));
    painter.drawText(QRect(2, plotRect.bottom() - 20, kLeftMargin - 8, 20),
                     Qt::AlignLeft | Qt::AlignBottom,
                     valueText(minVoltage));

    painter.setPen(kCurrentColor);
    painter.drawText(QRect(plotRect.right() + 8, plotRect.top() + 8, kRightMargin - 10, 20),
                     Qt::AlignRight | Qt::AlignTop,
                     valueText(maxCurrent));
    painter.drawText(QRect(plotRect.right() + 8, plotRect.top() + (plotRect.height() / 2) - 10, kRightMargin - 10, 20),
                     Qt::AlignRight | Qt::AlignVCenter,
                     valueText((minCurrent + maxCurrent) / 2.0));
    painter.drawText(QRect(plotRect.right() + 8, plotRect.bottom() - 20, kRightMargin - 10, 20),
                     Qt::AlignRight | Qt::AlignBottom,
                     valueText(minCurrent));

    painter.setPen(Qt::black);
    painter.drawText(QRect(plotRect.left(), plotRect.bottom() + 8, plotRect.width(), 20),
                     Qt::AlignCenter, "Time (s)");
    painter.setPen(kAxisColor);
    painter.drawText(QRect(plotRect.left(), plotRect.bottom() + 8, 80, 20),
                     Qt::AlignLeft, QString::number(minTime, 'f', 1));
    painter.drawText(QRect(plotRect.right() - 80, plotRect.bottom() + 8, 80, 20),
                     Qt::AlignRight, QString::number(maxTime, 'f', 1));
}

const std::vector<ChargerHistoryPlotWidget::Sample>&
ChargerHistoryPlotWidget::samplesForCharger(uint32_t chargerId) const
{
    static const std::vector<Sample> emptySamples;
    if (chargerId < 1 || chargerId > 3)
        return emptySamples;

    return m_history[static_cast<size_t>(chargerId - 1)];
}

double ChargerHistoryPlotWidget::paddedLowerBound(double minValue, double maxValue)
{
    const double span = std::max(0.1, std::abs(maxValue - minValue));
    return minValue - span * 0.1;
}

double ChargerHistoryPlotWidget::paddedUpperBound(double minValue, double maxValue)
{
    const double span = std::max(0.1, std::abs(maxValue - minValue));
    return maxValue + span * 0.1;
}
