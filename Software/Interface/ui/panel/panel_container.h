#pragma once

#include <QWidget>

class QPushButton;
class QLabel;
class QVBoxLayout;

class PanelContainer : public QWidget
{
    Q_OBJECT

public:
    explicit PanelContainer(const QString& title = QString(), QWidget* parent = nullptr);

    void setContentWidget(QWidget* widget);
    QWidget* contentWidget() const;

signals:
    void closeRequested(PanelContainer* self);

private:
    QLabel* m_titleLabel;
    QPushButton* m_closeButton;
    QVBoxLayout* m_contentLayout;
    QWidget* m_contentWidget = nullptr;
};