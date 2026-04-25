#pragma once

#include <QWidget>

#include <array>
#include <cstdint>
#include <vector>

class ChargerHistoryPlotWidget : public QWidget
{
public:
    explicit ChargerHistoryPlotWidget(QWidget* parent = nullptr);

    void clearHistory();
    void appendSample(uint32_t chargerId, double timeSeconds, double voltage, double current);
    void setSelectedCharger(uint32_t chargerId);
    uint32_t selectedCharger() const;
    int sampleCountForCharger(uint32_t chargerId) const;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    struct Sample
    {
        double timeSeconds = 0.0;
        double voltage = 0.0;
        double current = 0.0;
    };

    const std::vector<Sample>& samplesForCharger(uint32_t chargerId) const;
    static double paddedLowerBound(double minValue, double maxValue);
    static double paddedUpperBound(double minValue, double maxValue);

    static constexpr int kMaxSamples = 180;

    std::array<std::vector<Sample>, 3> m_history;
    uint32_t m_selectedCharger = 1;
};
