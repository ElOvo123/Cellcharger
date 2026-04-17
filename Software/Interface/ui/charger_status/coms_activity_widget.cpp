#include "coms_activity_widget.h"

#include <QPaintEvent>
#include <QPainter>
#include <QSizePolicy>

namespace
{
const QColor kCenterColor("#8e99a5");
const QColor kCenterDarkColor("#556270");
const QColor kFrameColor("#314252");
}

ComsActivityWidget::ComsActivityWidget(QWidget *parent)
    : QWidget(parent)
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

void ComsActivityWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF bounds = rect().adjusted(4, 4, -4, -4);
    const QPointF center = bounds.center();
    const qreal side = qMin(bounds.height() * 0.82, bounds.width() * 0.82);
    const QRectF centerSquare(center.x() - side / 2.0,
                              center.y() - side / 2.0,
                              side,
                              side);
    const QRectF centerInner = centerSquare.adjusted(side * 0.08,
                                                     side * 0.08,
                                                     -side * 0.08,
                                                     -side * 0.08);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 28));
    painter.drawRoundedRect(centerSquare.translated(side * 0.035, side * 0.045), 10.0, 10.0);

    painter.setPen(QPen(kFrameColor, 1.6));
    painter.setBrush(kCenterColor);
    painter.drawRoundedRect(centerSquare, 10.0, 10.0);

    painter.setPen(QPen(QColor(255, 255, 255, 90), 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(centerInner, 8.0, 8.0);

    painter.setPen(QPen(kCenterDarkColor, 1.2));
    const qreal ventLeft = centerInner.left() + centerInner.width() * 0.18;
    const qreal ventRight = centerInner.right() - centerInner.width() * 0.18;
    for (int i = 0; i < 3; ++i)
    {
        const qreal y = centerInner.top() + centerInner.height() * (0.28 + i * 0.18);
        painter.drawLine(QPointF(ventLeft, y), QPointF(ventRight, y));
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
