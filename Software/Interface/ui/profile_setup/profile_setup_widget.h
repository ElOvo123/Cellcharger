#pragma once

#include <QWidget>
#include "profile_plot_widget.h"

namespace Ui {
class ProfileSetupWidget;
}

class ProfileSetupWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProfileSetupWidget(QWidget *parent = nullptr);
    ~ProfileSetupWidget();

private:
    Ui::ProfileSetupWidget *ui;
    ProfilePlotWidget *m_plotWidget = nullptr;

    void addSetpointRow(int rowIndex);
    void connectSetpointRow(int rowIndex);

private slots:
    void addSetpoint();
    void removeSetpoint();
    void updatePlot();
    void saveProfile();
    void loadProfile();
    void startTest();
};