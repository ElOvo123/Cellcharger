#include "coms_activity_widget.h"

#include <QPaintEvent>
#include <QPainter>
#include <QSizePolicy>
#include <QtMath>

#include <array>
#include <cmath>

namespace
{
const QColor kCenterColor("#e8eef4");
const QColor kCenterDarkColor("#566575");
const QColor kFrameColor("#9fb3c5");
const QColor kFreshSlotColor("#39d98a");
const QColor kFreshSlotBorderColor("#178858");
const QColor kIdleSlotColor("#e35d6a");
const QColor kIdleSlotBorderColor("#8f2f3d");
const QColor kPinColor("#6f8193");
} // namespace

ComsActivityWidget::ComsActivityWidget(QWidget* parent) : QWidget(parent)
{
    setObjectName("comsActivityWidget");
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void ComsActivityWidget::setSlotFresh(int slotIndex, bool fresh)
{
    if (slotIndex < 0 || slotIndex >= static_cast<int>(m_slotFresh.size()))
        return;

    if (m_slotFresh[static_cast<size_t>(slotIndex)] == fresh)
        return;

    m_slotFresh[static_cast<size_t>(slotIndex)] = fresh;
    update();
    emit visualStateChanged();
}

void ComsActivityWidget::clearSlots()
{
    bool changed = false;
    for (bool& slotFresh : m_slotFresh)
    {
        if (!slotFresh)
            continue;

        slotFresh = false;
        changed = true;
    }

    if (!changed)
        return;

    update();
    emit visualStateChanged();
}

bool ComsActivityWidget::isSlotFresh(int slotIndex) const
{
    if (slotIndex < 0 || slotIndex >= static_cast<int>(m_slotFresh.size()))
        return false;

    return m_slotFresh[static_cast<size_t>(slotIndex)];
}

void ComsActivityWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF bounds = rect().adjusted(4, 4, -4, -4);
    const QPointF center = bounds.center();
    const qreal side = qMin(bounds.height() * 0.82, bounds.width() * 0.82);
    const QRectF centerSquare(center.x() - side / 2.0, center.y() - side / 2.0, side, side);
    const QRectF centerInner = centerSquare.adjusted(side * 0.14, side * 0.14, -side * 0.14, -side * 0.14);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 28));
    painter.drawRect(centerSquare.translated(side * 0.025, side * 0.035));

    painter.setPen(QPen(kPinColor, 2.0, Qt::SolidLine, Qt::SquareCap));
    const int pinCount = 6;
    const qreal pinSpacing = centerSquare.width() / (pinCount + 1);
    const qreal pinLength = side * 0.09;
    for (int i = 1; i <= pinCount; ++i)
    {
        const qreal x = centerSquare.left() + pinSpacing * i;
        painter.drawLine(QPointF(x, centerSquare.top() - pinLength), QPointF(x, centerSquare.top()));
        painter.drawLine(QPointF(x, centerSquare.bottom()), QPointF(x, centerSquare.bottom() + pinLength));

        const qreal y = centerSquare.top() + pinSpacing * i;
        painter.drawLine(QPointF(centerSquare.left() - pinLength, y), QPointF(centerSquare.left(), y));
        painter.drawLine(QPointF(centerSquare.right(), y), QPointF(centerSquare.right() + pinLength, y));
    }

    painter.setPen(QPen(kFrameColor, 1.6));
    painter.setBrush(kCenterColor);
    painter.drawRect(centerSquare);

    painter.setPen(QPen(QColor(255, 255, 255, 150), 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(centerInner);

    painter.setPen(QPen(kCenterDarkColor, 1.2));
    painter.drawText(centerInner, Qt::AlignCenter, "MCU");

    const qreal traceLeft = centerSquare.left() + centerSquare.width() * 0.16;
    const qreal traceRight = centerSquare.right() - centerSquare.width() * 0.16;
    for (int i = 0; i < 2; ++i)
    {
        const qreal y = centerSquare.top() + centerSquare.height() * (0.25 + i * 0.5);
        painter.drawLine(QPointF(traceLeft, y), QPointF(centerInner.left(), y));
        painter.drawLine(QPointF(centerInner.right(), y), QPointF(traceRight, y));
    }

    const qreal orbitRadius = side * 0.72;
    const qreal slotRadius = side * 0.12;
    const std::array<qreal, 3> slotAngles = {-150.0, -30.0, 90.0};
    for (size_t i = 0; i < m_slotFresh.size(); ++i)
    {
        const qreal angleRadians = qDegreesToRadians(slotAngles[i]);
        const QPointF slotCenter(center.x() + std::cos(angleRadians) * orbitRadius,
                                 center.y() + std::sin(angleRadians) * orbitRadius);
        const QRectF slotRect(slotCenter.x() - slotRadius, slotCenter.y() - slotRadius, slotRadius * 2.0,
                              slotRadius * 2.0);
        const bool fresh = m_slotFresh[i];
        const QColor fillColor = fresh ? kFreshSlotColor : kIdleSlotColor;
        const QColor borderColor = fresh ? kFreshSlotBorderColor : kIdleSlotBorderColor;

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 36));
        painter.drawRect(slotRect.translated(slotRadius * 0.1, slotRadius * 0.14));

        painter.setPen(QPen(borderColor, 1.6));
        painter.setBrush(fillColor);
        painter.drawRect(slotRect);

        painter.setPen(QPen(QColor(255, 255, 255, fresh ? 170 : 110), 1.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(slotRect.topLeft() + QPointF(slotRadius * 0.4, slotRadius * 0.35),
                         slotRect.topRight() + QPointF(-slotRadius * 0.4, slotRadius * 0.35));
    }
}

QSize ComsActivityWidget::minimumSizeHint() const
{
    return {290, 290};
}

QSize ComsActivityWidget::sizeHint() const
{
    return minimumSizeHint();
}
