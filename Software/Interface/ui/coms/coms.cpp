#include "coms.h"
#include "ui_coms.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QSerialPortInfo>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QSpacerItem>
#include <QToolButton>
#include <QVBoxLayout>

struct Coms::ConnectionRowWidgets
{
    QWidget* root = nullptr;
    QComboBox* typeCombo = nullptr;
    QStackedWidget* configStack = nullptr;
    QComboBox* serialPortCombo = nullptr;
    QLineEdit* baudrateEdit = nullptr;
    QComboBox* canInterfaceCombo = nullptr;
    QLineEdit* ipEdit = nullptr;
    QLineEdit* portEdit = nullptr;
    QLabel* wifiLabel = nullptr;
    QPushButton* connectButton = nullptr;
    QPushButton* removeButton = nullptr;
};

namespace
{
constexpr int kCompactControlHeight = 32;
constexpr int kCompactButtonSize = 36;
const char kComboBoxStyle[] = "QComboBox {"
                              "  padding: 0 10px;"
                              "  border: 1px solid #c7ccd4;"
                              "  border-radius: 5px;"
                              "  background: #ffffff;"
                              "  color: #1f2933;"
                              "}"
                              "QComboBox::drop-down {"
                              "  width: 22px;"
                              "  border: 0;"
                              "}"
                              "QComboBox QAbstractItemView {"
                              "  border: 1px solid #c7ccd4;"
                              "  background: #ffffff;"
                              "  color: #1f2933;"
                              "  selection-background-color: #dcecff;"
                              "  selection-color: #102a43;"
                              "  outline: 0;"
                              "}";

QIcon makeAddIcon()
{
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(QColor("#2f80ed"), 3, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(pen);
    painter.drawLine(12, 5, 12, 19);
    painter.drawLine(5, 12, 19, 12);

    return QIcon(pixmap);
}

QPixmap makeWifiPixmap(const QColor& color)
{
    QPixmap pixmap(22, 22);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, 2.0, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    painter.drawArc(3, 9, 16, 10, 30 * 16, 120 * 16);
    painter.drawArc(6, 12, 10, 6, 30 * 16, 120 * 16);
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(11.0, 17.0), 2.2, 2.2);

    return pixmap;
}
} // namespace

Coms::Coms(QWidget* parent) : QDialog(parent), ui(new Ui::Coms)
{
    ui->setupUi(this);

    ui->topActionBar->setStyleSheet("QWidget#topActionBar {"
                                    "  background-color: #e6e6e6;"
                                    "  border-bottom: 1px solid #c8c8c8;"
                                    "}");

    ui->addConnectionButton->setStyleSheet("QToolButton {"
                                           "  background-color: transparent;"
                                           "  color: #1f1f1f;"
                                           "  border: 1px solid #c8c8c8;"
                                           "  border-radius: 4px;"
                                           "  font-size: 12px;"
                                           "  font-weight: 500;"
                                           "  padding: 4px 10px;"
                                           "  text-align: center;"
                                           "}"
                                           "QToolButton:hover {"
                                           "  background-color: #f0f0f0;"
                                           "}"
                                           "QToolButton:pressed {"
                                           "  background-color: #e2e2e2;"
                                           "}");
    ui->addConnectionButton->setMinimumSize(56, 52);
    ui->addConnectionButton->setMaximumWidth(64);
    ui->addConnectionButton->setIcon(makeAddIcon());
    ui->addConnectionButton->setIconSize(QSize(24, 24));

    connect(ui->addConnectionButton, &QPushButton::clicked, this, &Coms::addConnectionRequested);

    setStatusText("No active connections");
}

Coms::~Coms()
{
    delete ui;
}

ComsType Coms::connectionType(int row) const
{
    if (row < 0 || row >= m_rowWidgets.size())
        return ComsType::Serial;

    return static_cast<ComsType>(m_rowWidgets[row]->typeCombo->currentData().toInt());
}

ComsConfig Coms::connectionConfig(int row) const
{
    ComsConfig config;
    if (row < 0 || row >= m_rowWidgets.size())
        return config;

    const ConnectionRowWidgets* widgets = m_rowWidgets[row];
    config.serialPort = widgets->serialPortCombo->currentText();
    config.baudrate = widgets->baudrateEdit->text().toInt();
    config.canInterface = widgets->canInterfaceCombo->currentText();
    config.ip = widgets->ipEdit->text();
    config.port = widgets->portEdit->text().toInt();
    return config;
}

void Coms::setConnections(const QList<ComsConnectionInfo>& connections)
{
    ensureConnectionRows(connections.size());

    for (int i = 0; i < connections.size(); ++i)
        updateRowWidget(i, connections[i]);
}

void Coms::setStatusText(const QString& text)
{
    setToolTip(text);
}

void Coms::setOverallConnected(bool connected)
{
    Q_UNUSED(connected);
}

void Coms::pulseReceiveActivity() {}

void Coms::ensureConnectionRows(int count)
{
    auto* layout = ui->connectionsListLayout;

    while (m_rowWidgets.size() < count)
    {
        auto* widgets = new ConnectionRowWidgets;
        widgets->root = new QFrame(ui->connectionsScrollWidget);
        widgets->root->setObjectName("connectionRowFrame");
        widgets->root->setMinimumHeight(52);
        widgets->root->setMaximumHeight(58);
        widgets->root->setStyleSheet("QFrame#connectionRowFrame {"
                                     "  background-color: #fbfbfb;"
                                     "  border: 1px solid #d7d7d7;"
                                     "  border-radius: 6px;"
                                     "}");

        auto* rootLayout = new QVBoxLayout(widgets->root);
        rootLayout->setContentsMargins(10, 6, 10, 6);
        rootLayout->setSpacing(4);

        auto* controlsLayout = new QHBoxLayout;
        controlsLayout->setContentsMargins(0, 0, 0, 0);
        controlsLayout->setSpacing(8);
        widgets->typeCombo = new QComboBox(widgets->root);
        widgets->typeCombo->setObjectName("typeCombo");
        widgets->typeCombo->addItem("Serial", static_cast<int>(ComsType::Serial));
        widgets->typeCombo->addItem("Socket vcan", static_cast<int>(ComsType::Socket_vcan));
        widgets->typeCombo->addItem("Socket UDP", static_cast<int>(ComsType::Socket_UDP));
        widgets->typeCombo->addItem("Socket TCP", static_cast<int>(ComsType::Socket_TCP));
        widgets->typeCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        widgets->typeCombo->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        widgets->typeCombo->setMinimumContentsLength(8);
        widgets->typeCombo->setFixedHeight(kCompactControlHeight);
        widgets->typeCombo->setStyleSheet(kComboBoxStyle);
        controlsLayout->addWidget(widgets->typeCombo);
        widgets->configStack = new QStackedWidget(widgets->root);
        widgets->configStack->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        widgets->configStack->setFixedHeight(kCompactControlHeight);

        auto* serialPage = new QWidget(widgets->configStack);
        auto* serialLayout = new QHBoxLayout(serialPage);
        serialLayout->setContentsMargins(0, 0, 0, 0);
        serialLayout->setSpacing(8);
        widgets->serialPortCombo = new QComboBox(serialPage);
        widgets->serialPortCombo->setObjectName("serialPortCombo");
        widgets->serialPortCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        widgets->serialPortCombo->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        widgets->serialPortCombo->setMinimumContentsLength(6);
        widgets->serialPortCombo->setFixedHeight(kCompactControlHeight);
        widgets->serialPortCombo->setStyleSheet(kComboBoxStyle);
        for (const QSerialPortInfo& info : QSerialPortInfo::availablePorts())
            widgets->serialPortCombo->addItem(info.portName());
        widgets->baudrateEdit = new QLineEdit(serialPage);
        widgets->baudrateEdit->setObjectName("baudrateEdit");
        widgets->baudrateEdit->setFixedHeight(kCompactControlHeight);
        widgets->baudrateEdit->setFixedWidth(90);
        widgets->baudrateEdit->setText("115200");
        widgets->baudrateEdit->setStyleSheet("QLineEdit {"
                                             "  padding: 0 10px;"
                                             "  border: 1px solid #c7ccd4;"
                                             "  border-radius: 5px;"
                                             "  background: #ffffff;"
                                             "}");
        serialLayout->addWidget(widgets->serialPortCombo);
        serialLayout->addWidget(new QLabel("Baudrate:", serialPage));
        serialLayout->addWidget(widgets->baudrateEdit);
        serialLayout->addStretch();
        widgets->configStack->addWidget(serialPage);

        auto* canPage = new QWidget(widgets->configStack);
        auto* canLayout = new QHBoxLayout(canPage);
        canLayout->setContentsMargins(0, 0, 0, 0);
        canLayout->setSpacing(8);
        widgets->canInterfaceCombo = new QComboBox(canPage);
        widgets->canInterfaceCombo->setObjectName("canInterfaceCombo");
        widgets->canInterfaceCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        widgets->canInterfaceCombo->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        widgets->canInterfaceCombo->setMinimumContentsLength(6);
        widgets->canInterfaceCombo->setFixedHeight(kCompactControlHeight);
        widgets->canInterfaceCombo->setStyleSheet(kComboBoxStyle);
        widgets->canInterfaceCombo->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        widgets->canInterfaceCombo->addItems(availableCanInterfaces());
        canLayout->addWidget(new QLabel("Socket:", canPage));
        canLayout->addWidget(widgets->canInterfaceCombo);
        canLayout->addStretch();
        widgets->configStack->addWidget(canPage);

        auto* networkPage = new QWidget(widgets->configStack);
        auto* networkLayout = new QHBoxLayout(networkPage);
        networkLayout->setContentsMargins(0, 0, 0, 0);
        networkLayout->setSpacing(8);
        widgets->ipEdit = new QLineEdit(networkPage);
        widgets->portEdit = new QLineEdit(networkPage);
        widgets->ipEdit->setObjectName("ipEdit");
        widgets->portEdit->setObjectName("portEdit");
        widgets->ipEdit->setFixedHeight(kCompactControlHeight);
        widgets->portEdit->setFixedHeight(kCompactControlHeight);
        widgets->ipEdit->setFixedWidth(180);
        widgets->portEdit->setFixedWidth(72);
        widgets->ipEdit->setStyleSheet(widgets->baudrateEdit->styleSheet());
        widgets->portEdit->setStyleSheet(widgets->baudrateEdit->styleSheet());
        networkLayout->addWidget(new QLabel("IP:", networkPage));
        networkLayout->addWidget(widgets->ipEdit);
        networkLayout->addWidget(new QLabel("Port:", networkPage));
        networkLayout->addWidget(widgets->portEdit);
        networkLayout->addStretch();
        widgets->configStack->addWidget(networkPage);

        widgets->connectButton = new QPushButton(widgets->root);
        widgets->removeButton = new QPushButton("X", widgets->root);
        widgets->wifiLabel = new QLabel(widgets->root);
        widgets->connectButton->setObjectName("connectButton");
        widgets->removeButton->setObjectName("removeButton");
        widgets->wifiLabel->setObjectName("wifiLabel");
        widgets->connectButton->setIcon(QIcon(":/images/chain.png"));
        widgets->connectButton->setToolTip("Connect");
        widgets->removeButton->setToolTip("Remove");
        widgets->connectButton->setFixedSize(kCompactButtonSize, kCompactButtonSize);
        widgets->removeButton->setFixedSize(kCompactButtonSize, kCompactButtonSize);
        widgets->wifiLabel->setFixedSize(30, 30);
        widgets->connectButton->setIconSize(QSize(20, 20));
        widgets->removeButton->setStyleSheet("QPushButton {"
                                             "  font-size: 16px;"
                                             "  font-weight: 700;"
                                             "  border: 1px solid #c7ccd4;"
                                             "  border-radius: 5px;"
                                             "  background: #ffffff;"
                                             "}"
                                             "QPushButton:hover {"
                                             "  background: #f5f7fa;"
                                             "}");
        widgets->connectButton->setStyleSheet("QPushButton {"
                                              "  border: 1px solid #c7ccd4;"
                                              "  border-radius: 5px;"
                                              "  background: #ffffff;"
                                              "}"
                                              "QPushButton:hover {"
                                              "  background: #f5f7fa;"
                                              "}");

        controlsLayout->addWidget(widgets->configStack);
        controlsLayout->addStretch();
        controlsLayout->addWidget(widgets->removeButton);
        controlsLayout->addWidget(widgets->connectButton);
        controlsLayout->addWidget(widgets->wifiLabel);
        rootLayout->addLayout(controlsLayout);

        connect(widgets->typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), widgets->root,
                [widgets](int index)
                {
                    const auto type = static_cast<ComsType>(widgets->typeCombo->itemData(index).toInt());
                    widgets->configStack->setCurrentIndex(configPageIndexForType(type));
                });

        connect(widgets->connectButton, &QPushButton::clicked, this,
                [this, widgets]()
                {
                    const int row = widgets->root->property("row").toInt();
                    const bool connected = widgets->root->property("connected").toBool();
                    if (connected)
                        emit disconnectConnectionRequested(row);
                    else
                        emit connectConnectionRequested(row);
                });
        connect(widgets->removeButton, &QPushButton::clicked, this,
                [this, widgets]() { emit removeConnectionRequested(widgets->root->property("row").toInt()); });

        layout->insertWidget(m_rowWidgets.size(), widgets->root);
        m_rowWidgets.append(widgets);
    }

    while (m_rowWidgets.size() > count)
    {
        ConnectionRowWidgets* widgets = m_rowWidgets.takeLast();
        layout->removeWidget(widgets->root);
        delete widgets->root;
        delete widgets;
    }

    if (layout->count() == count)
        layout->addStretch();

    rebuildRowIndices();
}

void Coms::rebuildRowIndices()
{
    for (int i = 0; i < m_rowWidgets.size(); ++i)
        m_rowWidgets[i]->root->setProperty("row", i);
}

void Coms::updateRowWidget(int row, const ComsConnectionInfo& connection)
{
    if (row < 0 || row >= m_rowWidgets.size())
        return;

    ConnectionRowWidgets* widgets = m_rowWidgets[row];

    const int typeIndex = widgets->typeCombo->findData(static_cast<int>(connection.type));
    if (typeIndex >= 0)
        widgets->typeCombo->setCurrentIndex(typeIndex);

    widgets->serialPortCombo->setCurrentText(connection.config.serialPort);
    widgets->baudrateEdit->setText(QString::number(connection.config.baudrate));
    widgets->canInterfaceCombo->setCurrentText(normalizedCanInterface(connection.config.canInterface));
    widgets->ipEdit->setText(connection.config.ip);
    widgets->portEdit->setText(QString::number(connection.config.port));

    widgets->root->setProperty("connected", connection.connected);
    widgets->wifiLabel->setPixmap(makeWifiPixmap(connection.indicatorColor));
    widgets->connectButton->setEnabled(true);
    widgets->connectButton->setToolTip(connection.connected ? "Disconnect" : "Connect");
}
