#include "panel_container.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

PanelContainer::PanelContainer(const QString& title, QWidget *parent) : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);

    auto *topBarLayout = new QHBoxLayout();
    topBarLayout->setContentsMargins(4, 4, 4, 0);

    m_titleLabel = new QLabel(title, this);

    m_closeButton = new QPushButton("×", this);
    m_closeButton->setFixedSize(22, 22);
    m_closeButton->setCursor(Qt::PointingHandCursor);
    m_closeButton->setToolTip("Close panel");
    m_closeButton->setStyleSheet( "QPushButton { border:none; font-size:16px; }" "QPushButton:hover { background:#d9d9d9; }" );

    topBarLayout->addWidget(m_titleLabel);
    topBarLayout->addStretch();
    topBarLayout->addWidget(m_closeButton);

    QWidget *contentHost = new QWidget(this);
    m_contentLayout = new QVBoxLayout(contentHost);
    m_contentLayout->setContentsMargins(0, 0, 0, 0);

    mainLayout->addLayout(topBarLayout);
    mainLayout->addWidget(contentHost);

    connect(m_closeButton, &QPushButton::clicked, this, [this]() { emit closeRequested(this); });
}

void PanelContainer::setContentWidget(QWidget *widget)
{
    if (!widget) 
    {
        return;
    }

    if (m_contentWidget) 
    {
        m_contentWidget->setParent(nullptr);
    }

    m_contentWidget = widget;
    m_contentLayout->addWidget(m_contentWidget);
}

QWidget* PanelContainer::contentWidget() const
{
    return m_contentWidget;
}