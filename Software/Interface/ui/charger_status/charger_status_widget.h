#pragma once

#include <QElapsedTimer>
#include <QWidget>

#include <array>

namespace Ui
{
class ChargerStatusWidget;
}

class ComsActivityWidget;
class ChargerHistoryPlotWidget;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QTimer;
class QTabWidget;

class ChargerStatusWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChargerStatusWidget(QWidget* parent = nullptr);
    ~ChargerStatusWidget();

signals:
    void commandRequested(uint32_t chargerId, int mode, bool start, double setpoint);

private:
    struct SlotWidgets
    {
        QWidget* led = nullptr;
        QLabel* overviewLabel = nullptr;
        QLabel* generalTitleLabel = nullptr;
        QLabel* generalVoltageLabel = nullptr;
        QLabel* generalCurrentLabel = nullptr;
        QLabel* generalTempLabel = nullptr;
        QPushButton* startButton = nullptr;
        QPushButton* irButton = nullptr;
        QPushButton* ecmButton = nullptr;
        QPushButton* capacityButton = nullptr;
        QPushButton* enduranceButton = nullptr;
        QTimer* timer = nullptr;
        uint32_t deviceId = 0;
        bool assigned = false;
        bool fresh = false;
    };

    void setupTabWidget();
    void setupCommandButtons();
    void setupDetailedControls();
    uint32_t selectedDetailedChargerId() const;
    void updateDetailedSetpointLabel();
    void clearSlots();
    void applySlotState(int slotIndex, bool active);
    void updateCommandEnablement();
    bool isSlotFresh(int slotIndex) const;
    int slotIndexForDevice(uint32_t deviceId);
    void updateSlotLabel(int slotIndex, const QString& voltText, const QString& currentText, const QString& tempText,
                         const QString& statusText);
    void markSlotFresh(int slotIndex);
    void processDecodedMessage(const QString& message);
    void emitCommandForSlot(int slotIndex, int mode, bool start);
    void emitDetailedCommand(bool start);
    void appendHistorySample(uint32_t chargerId, const QMap<QString, QString>& signalValues);

private:
    Ui::ChargerStatusWidget* ui = nullptr;
    QTabWidget* m_tabWidget = nullptr;
    ComsActivityWidget* m_activityWidget = nullptr;
    std::array<ChargerHistoryPlotWidget*, 3> m_historyPlots = {};
    QElapsedTimer m_historyTimer;
    std::array<SlotWidgets, 3> m_slots;
};
