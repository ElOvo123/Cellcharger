#pragma once

#include <QWidget>

#include <array>

class QTimer;

class ComsActivityWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ComsActivityWidget(QWidget* parent = nullptr);

    void setSlotFresh(int slotIndex, bool fresh);
    void clearSlots();

    bool isSlotFresh(int slotIndex) const;

signals:
    void visualStateChanged();

protected:
    void paintEvent(QPaintEvent* event) override;
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

private:
    std::array<bool, 3> m_slotFresh = {false, false, false};
};
