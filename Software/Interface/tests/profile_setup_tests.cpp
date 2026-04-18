#include <QtTest>
#include <QPixmap>

#define private public
#include "../ui/profile_setup/profile_plot_widget.h"
#undef private

class ProfileSetupTests : public QObject
{
    Q_OBJECT

private slots:
    void interpolateLinearReturnsMidpoint();
    void interpolateExponentialReturnsExponentialValue();
    void interpolateRampStepsUpInDiscreteIncrements();
    void paddedBoundsReturnOneUnitMinimumPadding();
    void renderPlotWidgetWithoutCrashing();
};

void ProfileSetupTests::interpolateLinearReturnsMidpoint()
{
    ProfilePlotWidget widget;
    const double value = widget.interpolate(0.0, 0.0, 10.0, 10.0, 5.0, "Linear", 1.0);
    QCOMPARE(value, 5.0);
}

void ProfileSetupTests::interpolateExponentialReturnsExponentialValue()
{
    ProfilePlotWidget widget;
    const double value = widget.interpolate(0.0, 1.0, 10.0, 100.0, 5.0, "Exponential", 1.0);
    QCOMPARE(value, 10.0);
}

void ProfileSetupTests::interpolateRampStepsUpInDiscreteIncrements()
{
    ProfilePlotWidget widget;
    QCOMPARE(widget.interpolate(0.0, 0.0, 10.0, 10.0, 2.4, "Ramp", 3.0), 0.0);
    QCOMPARE(widget.interpolate(0.0, 0.0, 10.0, 10.0, 2.5, "Ramp", 3.0), 3.0);
    QCOMPARE(widget.interpolate(0.0, 0.0, 10.0, 10.0, 9.9, "Ramp", 3.0), 9.0);
    QCOMPARE(widget.interpolate(0.0, 0.0, 10.0, 10.0, 10.0, "Ramp", 3.0), 10.0);
}

void ProfileSetupTests::paddedBoundsReturnOneUnitMinimumPadding()
{
    QCOMPARE(ProfilePlotWidget::paddedLowerBound(0.0, 10.0), -1.0);
    QCOMPARE(ProfilePlotWidget::paddedUpperBound(0.0, 10.0), 11.0);
}

void ProfileSetupTests::renderPlotWidgetWithoutCrashing()
{
    ProfilePlotWidget widget;
    widget.resize(600, 300);

    std::vector<Setpoint> setpoints = {
        {0.0, 0.0, 0.0, 25.0, "Linear", 1.0},
        {10.0, 10.0, 5.0, 30.0, "Linear", 1.0}
    };
    widget.setSetpoints(setpoints);
    widget.setDisplayMode(ProfilePlotWidget::DisplayMode::All);

    QPixmap pix(widget.size());
    pix.fill(Qt::transparent);
    widget.render(&pix);

    QVERIFY(!pix.isNull());
    QCOMPARE(pix.size(), widget.size());
}

QTEST_MAIN(ProfileSetupTests)
#include "profile_setup_tests.moc"
