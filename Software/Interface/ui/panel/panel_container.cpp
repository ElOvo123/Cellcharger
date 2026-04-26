#include "panel_container.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSizePolicy>
#include <QVBoxLayout>

PanelContainer::PanelContainer(const QString& title, QWidget* parent) : QWidget(parent)
{
    setObjectName("panelContainer");
    setMinimumWidth(0);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("QWidget#panelContainer {"
                  "  background-color: #ffffff;"
                  "  border: 1px solid #d6dee6;"
                  "  border-radius: 2px;"
                  "}"
                  "QWidget#panelTopBar {"
                  "  background-color: #f2f6fa;"
                  "  border-bottom: 1px solid #d6dee6;"
                  "  border-top-left-radius: 2px;"
                  "  border-top-right-radius: 2px;"
                  "}"
                  "QLabel#panelTitleLabel {"
                  "  color: #17212b;"
                  "  font-size: 10pt;"
                  "  font-weight: 700;"
                  "}"
                  "QPushButton#panelCloseButton {"
                  "  color: #566575;"
                  "  background-color: transparent;"
                  "  border: 1px solid transparent;"
                  "  border-radius: 1px;"
                  "  font-size: 15px;"
                  "  font-weight: 600;"
                  "  padding: 0;"
                  "}"
                  "QPushButton#panelCloseButton:hover {"
                  "  color: #ffffff;"
                  "  background-color: #8f2f3d;"
                  "  border-color: #b94a5a;"
                  "}");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* topBar = new QWidget(this);
    topBar->setObjectName("panelTopBar");
    auto* topBarLayout = new QHBoxLayout(topBar);
    topBarLayout->setContentsMargins(12, 8, 8, 8);
    topBarLayout->setSpacing(8);

    m_titleLabel = new QLabel(title, this);
    m_titleLabel->setObjectName("panelTitleLabel");

    m_closeButton = new QPushButton("x", this);
    m_closeButton->setObjectName("panelCloseButton");
    m_closeButton->setFixedSize(24, 24);
    m_closeButton->setCursor(Qt::PointingHandCursor);
    m_closeButton->setToolTip("Close panel");

    topBarLayout->addWidget(m_titleLabel);
    topBarLayout->addStretch();
    topBarLayout->addWidget(m_closeButton);

    QWidget* contentHost = new QWidget(this);
    contentHost->setObjectName("panelContentHost");
    contentHost->setMinimumWidth(0);
    contentHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_contentLayout = new QVBoxLayout(contentHost);
    m_contentLayout->setContentsMargins(0, 0, 0, 0);

    mainLayout->addWidget(topBar);
    mainLayout->addWidget(contentHost);

    connect(m_closeButton, &QPushButton::clicked, this, [this]() { emit closeRequested(this); });
}

void PanelContainer::setContentWidget(QWidget* widget)
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
    m_contentWidget->setMinimumWidth(0);
    m_contentWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
    m_contentLayout->addWidget(m_contentWidget);
}

QWidget* PanelContainer::contentWidget() const
{
    return m_contentWidget;
}
