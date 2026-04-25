#pragma once

#include <QWidget>
#include <QIcon>
#include <QTableWidget>
#include <QTimer>
#include <array>
#include <memory>
#include "profile_plot_widget.h"
#include "profile_setup_dialogs.h"
#include "profile_yaml_logic.h"

namespace Ui
{
class ProfileSetupWidget;
}

class ProfileSetupWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProfileSetupWidget(QWidget* parent = nullptr, ProfileSetupDialogs* dialogs = nullptr);
    ~ProfileSetupWidget();

signals:
    void commandRequested(uint32_t chargerId, int mode, bool start, double setpoint);

private:
    struct SlotRuntime
    {
        QTimer* timer = nullptr;
        QTableWidget* table = nullptr;
        int activeRow = -1;
        int elapsedSeconds = 0;
        bool paused = false;
    };

    Ui::ProfileSetupWidget* ui;
    std::unique_ptr<ProfileSetupDialogs> m_ownedDialogs;
    ProfileSetupDialogs* m_dialogs = nullptr;
    ProfilePlotWidget* m_plotWidget1 = nullptr;
    ProfilePlotWidget* m_plotWidget2 = nullptr;
    ProfilePlotWidget* m_plotWidget3 = nullptr;
    QIcon m_activeStepIcon;
    std::array<SlotRuntime, 3> m_slotRuntimes;

    void addSetpointRow(int slotIndex, int rowIndex);
    void setupSlot(int slotIndex);
    void updatePlotForSlot(int slotIndex, QTableWidget* table, ProfilePlotWidget* plotWidget);
    ProfileDocument profileDocumentFromUi() const;
    void applyProfileDocument(const ProfileDocument& document);
    void applySetpointsToTable(QTableWidget* table, const std::vector<Setpoint>& setpoints,
                               void (ProfileSetupWidget::*updateSlot)());
    QString serializeProfileToYaml() const;
    bool deserializeProfileFromYaml(const QString& yamlText, QString* errorMessage = nullptr);
    void startTestForSlot(int slotIndex, QTableWidget* table);
    void pauseTestForSlot(int slotIndex);
    void resetTestForSlot(int slotIndex);
    void stopTestForSlot(int slotIndex);
    void updateActiveStepIndicator(int slotIndex);
    int activeStepRowForElapsedSeconds(QTableWidget* table, int elapsedSeconds) const;
    ProfilePlotWidget* plotWidgetForSlot(int slotIndex) const;
    int commandModeForSlot(int slotIndex) const;
    std::vector<Setpoint> setpointsForTable(QTableWidget* table) const;
    static double displayValueForMode(const Setpoint& setpoint, ProfilePlotWidget::DisplayMode mode);
    static double interpolateProfileValue(const std::vector<Setpoint>& setpoints, double timeSeconds,
                                          ProfilePlotWidget::DisplayMode mode);
    void showError(const QString& title, const QString& message);
    void showInfo(const QString& title, const QString& message);
    void showWarning(const QString& title, const QString& message);

private slots:
    void addSetpoint1();
    void removeSetpoint1();
    void updatePlot1();
    void addSetpoint2();
    void removeSetpoint2();
    void updatePlot2();
    void addSetpoint3();
    void removeSetpoint3();
    void updatePlot3();
    void saveProfile();
    void loadProfile();
    void startTest1();
    void startTest2();
    void startTest3();
    void pauseTest1();
    void pauseTest2();
    void pauseTest3();
    void resetTest1();
    void resetTest2();
    void resetTest3();
};
