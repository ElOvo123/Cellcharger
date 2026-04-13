#pragma once

#include <QWidget>

#include <array>

namespace Ui {
class ChargerStatusWidget;
}

class ComsActivityWidget;
class QLabel;
class QTimer;
class QTabWidget;

class ChargerStatusWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChargerStatusWidget(QWidget *parent = nullptr);
    ~ChargerStatusWidget();

private:
    struct SlotWidgets
    {
        QWidget *led = nullptr;
        QLabel *overviewLabel = nullptr;
        QLabel *generalTitleLabel = nullptr;
        QLabel *generalVoltageLabel = nullptr;
        QLabel *generalCurrentLabel = nullptr;
        QLabel *generalTempLabel = nullptr;
        QTimer *timer = nullptr;
        uint32_t deviceId = 0;
        bool assigned = false;
    };

    void setupTabWidget();
    void clearSlots();
    void applySlotState(int slotIndex, bool active);
    int slotIndexForDevice(uint32_t deviceId);
    void updateSlotLabel(int slotIndex,
                         const QString& voltText,
                         const QString& currentText,
                         const QString& tempText,
                         const QString& statusText);
    void markSlotFresh(int slotIndex);
    void processDecodedMessage(const QString& message);

private:
    Ui::ChargerStatusWidget *ui = nullptr;
    QTabWidget *m_tabWidget = nullptr;
    ComsActivityWidget *m_activityWidget = nullptr;
    std::array<SlotWidgets, 3> m_slots;
};
