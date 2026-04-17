#include "console_widget.h"
#include "ui_console_widget.h"
#include "logger_backend.h"
#include "pcp_decode_formatter.h"
#include "pcp_formatter.h"

#include <QComboBox>
#include <QEvent>
#include <QHeaderView>
#include <QMouseEvent>
#include <QPushButton>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>

namespace
{
double simulatedTxValueForSignal(const std::string& signalName,
                                 uint32_t deviceId,
                                 int counter)
{
    if (signalName == "charger_id" || signalName == "slot_id")
        return static_cast<double>(deviceId);

    if (signalName == "voltage")
        return 12.0 + (counter % 30) * 0.05 + static_cast<double>(deviceId) * 0.1;

    if (signalName == "cell_voltage")
        return 4.0 + (counter % 20) * 0.002 + static_cast<double>(deviceId) * 0.01;

    if (signalName == "current")
        return -2.0 + (counter % 20) * 0.2;

    if (signalName == "cell_current")
        return -0.5 + (counter % 20) * 0.025;

    if (signalName == "temperature")
        return 25.0 + (counter % 10) + static_cast<double>(deviceId);

    if (signalName == "cell_temp")
        return 24.0 + (counter % 8) * 0.125 + static_cast<double>(deviceId) * 0.1;

    if (signalName == "state" || signalName == "status")
        return counter % 3;

    if (signalName == "fault" || signalName == "fault_code")
        return (counter % 3 == 2) ? static_cast<double>((counter / 3) % 16) : 0.0;

    if (signalName == "enabled")
        return counter % 2;

    if (signalName == "mode")
        return counter % 4;

    if (signalName == "setpoint")
        return 4.1 + (counter % 10) * 0.01;

    if (signalName == "start")
        return counter % 2;

    if (signalName == "speed")
        return 1000.0 + (counter % 50) * 10.0;

    if (signalName == "power")
        return 50.0 + (counter % 25) * 1.5;

    return static_cast<double>((counter + deviceId) % 100);
}
}

const PCPDatabase* ConsoleWidget::s_sharedPCPDatabase = nullptr;

void ConsoleWidget::setSharedPCPDatabase(const PCPDatabase* pcpDatabase)
{
    s_sharedPCPDatabase = pcpDatabase;
}

ConsoleWidget::ConsoleWidget(QWidget *parent)
    : ConsoleWidget(s_sharedPCPDatabase, parent)
{
}

ConsoleWidget::ConsoleWidget(const PCPDatabase* pcpDatabase, QWidget *parent)
    : QWidget(parent),
      ui(new Ui::ConsoleWidget),
      m_pcpDatabase(pcpDatabase),
      m_messageModel(new ConsoleMessageModel(this)),
      m_messageProxyModel(new ConsoleMessageFilterProxyModel(this)),
      m_txEncoder(pcpDatabase),
      m_txDecoder(pcpDatabase)
{
    ui->setupUi(this);

    ui->pauseButton->setCheckable(true);

    setupMessageView();
    setupSignalView();
    setupFilterView();
    setupTxView();
    setupSignalSelectors();

    connect(ui->pauseButton, &QPushButton::toggled,
            this, &ConsoleWidget::onPauseToggled);

    connect(ui->clearButton, &QPushButton::clicked,
            this, &ConsoleWidget::onClearClicked);

    connect(ui->addSignalButton, &QPushButton::clicked,
            this, &ConsoleWidget::onAddSignalClicked);

    connect(ui->removeSignalButton, &QPushButton::clicked,
            this, &ConsoleWidget::onRemoveSignalClicked);

    connect(ui->clearSignalSelectionButton, &QPushButton::clicked,
            this, &ConsoleWidget::onClearSignalSelectionClicked);

    connect(ui->deviceComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &ConsoleWidget::onDeviceChanged);

    connect(ui->messageComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &ConsoleWidget::onMessageChanged);

    connect(ui->addFilterButton, &QPushButton::clicked,
            this, &ConsoleWidget::onAddFilterClicked);

    connect(ui->removeFilterButton, &QPushButton::clicked,
            this, &ConsoleWidget::onRemoveFilterClicked);

    connect(ui->filterTypeComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &ConsoleWidget::onFilterTypeChanged);

    connect(ui->txDeviceComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &ConsoleWidget::onTxDeviceChanged);

    connect(ui->txMessageComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &ConsoleWidget::onTxMessageChanged);

    connect(ui->sendTxButton, &QPushButton::clicked,
            this, &ConsoleWidget::onSendTxClicked);

    connect(ui->addPeriodicTxButton, &QPushButton::clicked,
            this, &ConsoleWidget::onAddPeriodicTxClicked);

    connect(ui->startPeriodicTxButton, &QPushButton::clicked,
            this, &ConsoleWidget::onStartPeriodicTxClicked);

    connect(ui->stopPeriodicTxButton, &QPushButton::clicked,
            this, &ConsoleWidget::onStopPeriodicTxClicked);

    connect(ui->removePeriodicTxButton, &QPushButton::clicked,
            this, &ConsoleWidget::onRemovePeriodicTxClicked);

    connect(ui->txIntervalSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &ConsoleWidget::onTxPeriodChanged);

    connect(&m_txTimer, &QTimer::timeout,
            this, &ConsoleWidget::onTxTimerTimeout);

    connect(ui->txPeriodicTable, &QTableWidget::cellDoubleClicked,
            this, &ConsoleWidget::onPeriodicTxRowDoubleClicked);

    for (const QString &line : Logger::instance().comsHistory())
    {
        m_allMessages.append(line);

        const auto parsed = parseMessageRecord(line);
        if (parsed.has_value())
            m_messageRecords.append(*parsed);
    }

    enforceVisibleMessageCapacity();
    m_messageModel->setRecords(m_messageRecords);

    refreshFilterDropdownOptions();
    refreshFilterValueWidget();
    refreshView();

    connect(&Logger::instance(),
            &Logger::newComsMessage,
            this,
            &ConsoleWidget::appendMessage,
            Qt::QueuedConnection);

    connect(&Logger::instance(),
            &Logger::newDecodedMessage,
            this,
            &ConsoleWidget::onStatusMessage,
            Qt::QueuedConnection);
}

ConsoleWidget::~ConsoleWidget()
{
    delete ui;
}

void ConsoleWidget::setupMessageView()
{
    m_messageProxyModel->setSourceModel(m_messageModel);
    m_messageProxyModel->setDynamicSortFilter(true);

    ui->consoleTableView->setModel(m_messageProxyModel);
    ui->consoleTableView->setSortingEnabled(true);
    ui->consoleTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->consoleTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->consoleTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->consoleTableView->setAlternatingRowColors(false);
    ui->consoleTableView->setShowGrid(false);
    ui->consoleTableView->setWordWrap(false);
    ui->consoleTableView->setTextElideMode(Qt::ElideNone);
    ui->consoleTableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerItem);
    ui->consoleTableView->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    ui->consoleTableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->consoleTableView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    ui->consoleTableView->verticalHeader()->setVisible(false);
    ui->consoleTableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->consoleTableView->verticalHeader()->setDefaultSectionSize(
        ui->consoleTableView->fontMetrics().height() + 6);

    updateMessageTableLayout();

    ui->messagesHeaderBar->setStyleSheet(
        "QWidget#messagesHeaderBar {"
        "  background-color: #e6e6e6;"
        "  border-bottom: 1px solid #c8c8c8;"
        "}"
    );

    ui->messagesBottomBar->setStyleSheet(
        "QWidget#messagesBottomBar {"
        "  background-color: #e6e6e6;"
        "  border-top: 1px solid #c8c8c8;"
        "}"
    );
}

void ConsoleWidget::setupSignalView()
{
    ui->signalsTable->horizontalHeader()->setStretchLastSection(true);
    ui->signalsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->signalsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->signalsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->signalsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    ui->signalsTable->verticalHeader()->setVisible(false);

    ui->signalsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->signalsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->signalsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->signalsTable->setAlternatingRowColors(true);
    ui->signalsTable->viewport()->installEventFilter(this);

    ui->signalTopBar->setStyleSheet(
        "QWidget#signalTopBar {"
        "  background-color: #e6e6e6;"
        "  border-bottom: 1px solid #c8c8c8;"
        "}"
    );

    ui->messageComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    ui->signalComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    ui->messageComboBox->setMinimumContentsLength(10);
    ui->signalComboBox->setMinimumContentsLength(14);
}

void ConsoleWidget::setupFilterView()
{
    ui->filtersTable->horizontalHeader()->setStretchLastSection(true);
    ui->filtersTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->filtersTable->verticalHeader()->setVisible(false);

    ui->filtersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->filtersTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->filtersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->filtersTable->setAlternatingRowColors(true);
    ui->filtersTable->viewport()->installEventFilter(this);

    ui->filterTopBar->setStyleSheet(
        "QWidget#filterTopBar {"
        "  background-color: #e6e6e6;"
        "  border-bottom: 1px solid #c8c8c8;"
        "}"
    );
}

void ConsoleWidget::setupTxView()
{
    ui->txTopBar->setStyleSheet(
        "QWidget#txTopBar {"
        "  background-color: #e6e6e6;"
        "  border-bottom: 1px solid #c8c8c8;"
        "}"
    );

    ui->txPeriodicBar->setStyleSheet(
        "QWidget#txPeriodicBar {"
        "  background-color: #e6e6e6;"
        "  border-bottom: 1px solid #c8c8c8;"
        "}"
    );

    ui->txDeviceComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    ui->txMessageComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    ui->txDeviceComboBox->setMinimumContentsLength(14);
    ui->txMessageComboBox->setMinimumContentsLength(14);
    ui->txSignalsTable->horizontalHeader()->setStretchLastSection(true);
    ui->txSignalsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->txSignalsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->txSignalsTable->verticalHeader()->setVisible(false);
    ui->txSignalsTable->setSelectionMode(QAbstractItemView::NoSelection);
    ui->txSignalsTable->setSelectionBehavior(QAbstractItemView::SelectItems);
    ui->txPeriodicTable->horizontalHeader()->setStretchLastSection(true);
    ui->txPeriodicTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->txPeriodicTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->txPeriodicTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->txPeriodicTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    ui->txPeriodicTable->verticalHeader()->setVisible(false);
    ui->txPeriodicTable->viewport()->installEventFilter(this);
    ui->txStatusLabel->setVisible(false);

    ui->txDeviceComboBox->clear();
    ui->txDeviceComboBox->addItem("");

    if (m_pcpDatabase)
    {
        for (uint32_t deviceId : m_pcpDatabase->deviceIds())
            ui->txDeviceComboBox->addItem(deviceDisplayName(deviceId));
    }

    ui->txDeviceComboBox->setCurrentIndex(0);
    refreshTxMessageCombo();
    refreshTxSignalTable();
    refreshPeriodicTxTable();
    m_txTimer.setInterval(100);
}

QString ConsoleWidget::deviceDisplayName(uint32_t deviceId) const
{
    if (!m_pcpDatabase)
        return QString("Device %1").arg(deviceId);

    return QString("%1 (%2)")
        .arg(QString::fromStdString(m_pcpDatabase->deviceName(deviceId)))
        .arg(deviceId);
}

std::optional<uint32_t> ConsoleWidget::deviceIdFromDisplayName(const QString& displayName) const
{
    QRegularExpression re(R"(\((\d+)\)\s*$)");
    QRegularExpressionMatch match = re.match(displayName);
    if (!match.hasMatch())
        return std::nullopt;

    return match.captured(1).toUInt();
}

void ConsoleWidget::setupSignalSelectors()
{
    ui->deviceComboBox->clear();
    ui->deviceComboBox->addItem("");

    if (m_pcpDatabase)
    {
        for (uint32_t deviceId : m_pcpDatabase->deviceIds())
            ui->deviceComboBox->addItem(deviceDisplayName(deviceId));
    }

    ui->deviceComboBox->setCurrentIndex(0);

    refreshMessageCombo();
    refreshSignalCombo();
}

void ConsoleWidget::refreshMessageCombo()
{
    const QString currentMessage = ui->messageComboBox->currentText();
    const QString deviceText = ui->deviceComboBox->currentText();

    ui->messageComboBox->clear();

    const auto deviceIdOpt = deviceIdFromDisplayName(deviceText);
    if (deviceIdOpt.has_value() && m_pcpDatabase)
    {
        auto devOpt = m_pcpDatabase->device(*deviceIdOpt);
        if (devOpt.has_value())
        {
            for (const auto& [messageName, _] : devOpt->messagesByName)
                ui->messageComboBox->addItem(QString::fromStdString(messageName));
        }
    }

    const int idx = ui->messageComboBox->findText(currentMessage);
    if (idx >= 0)
        ui->messageComboBox->setCurrentIndex(idx);
    else if (ui->messageComboBox->count() > 0)
        ui->messageComboBox->setCurrentIndex(0);
    else
        ui->messageComboBox->setCurrentIndex(-1);
}

void ConsoleWidget::refreshSignalCombo()
{
    const QString currentSignal = ui->signalComboBox->currentText();
    const QString deviceText = ui->deviceComboBox->currentText();
    const QString messageText = ui->messageComboBox->currentText();

    ui->signalComboBox->clear();

    const auto deviceIdOpt = deviceIdFromDisplayName(deviceText);
    if (deviceIdOpt.has_value() && m_pcpDatabase && !messageText.isEmpty())
    {
        const auto names = m_pcpDatabase->signalNames(*deviceIdOpt, messageText.toStdString());
        for (const std::string& name : names)
            ui->signalComboBox->addItem(QString::fromStdString(name));
    }

    const int signalIndex = ui->signalComboBox->findText(currentSignal);
    if (signalIndex >= 0)
        ui->signalComboBox->setCurrentIndex(signalIndex);
    else if (ui->signalComboBox->count() > 0)
        ui->signalComboBox->setCurrentIndex(0);
    else
        ui->signalComboBox->setCurrentIndex(-1);
}

void ConsoleWidget::refreshTxMessageCombo()
{
    const QString currentMessage = ui->txMessageComboBox->currentText();
    const QString deviceText = ui->txDeviceComboBox->currentText();

    ui->txMessageComboBox->clear();
    ui->txMessageComboBox->addItem("");

    const auto deviceIdOpt = deviceIdFromDisplayName(deviceText);
    if (deviceIdOpt.has_value() && m_pcpDatabase)
    {
        auto devOpt = m_pcpDatabase->device(*deviceIdOpt);
        if (devOpt.has_value())
        {
            for (const auto& [messageName, _] : devOpt->messagesByName)
                ui->txMessageComboBox->addItem(QString::fromStdString(messageName));
        }
    }

    const int index = ui->txMessageComboBox->findText(currentMessage);
    if (index >= 0)
        ui->txMessageComboBox->setCurrentIndex(index);
    else
        ui->txMessageComboBox->setCurrentIndex(0);
}

void ConsoleWidget::refreshTxSignalTable()
{
    ui->txSignalsTable->setRowCount(0);

    if (!m_pcpDatabase)
        return;

    const QString deviceText = ui->txDeviceComboBox->currentText().trimmed();
    const QString messageText = ui->txMessageComboBox->currentText().trimmed();
    const auto deviceIdOpt = deviceIdFromDisplayName(deviceText);

    if (!deviceIdOpt.has_value() || messageText.isEmpty())
        return;

    const PCPMessageDefinition* msgDef =
        m_pcpDatabase->messageByName(*deviceIdOpt, messageText.toStdString());

    if (!msgDef)
        return;

    ++m_txCounter;

    for (const auto& [signalName, _] : msgDef->signalDefinitions)
    {
        const int row = ui->txSignalsTable->rowCount();
        ui->txSignalsTable->insertRow(row);

        auto* signalItem = new QTableWidgetItem(QString::fromStdString(signalName));
        signalItem->setFlags(signalItem->flags() & ~Qt::ItemIsEditable);
        ui->txSignalsTable->setItem(row, 0, signalItem);

        auto* valueItem = new QTableWidgetItem(
            QString::number(simulatedTxValueForSignal(signalName, *deviceIdOpt, m_txCounter)));
        valueItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->txSignalsTable->setItem(row, 1, valueItem);
    }
}

void ConsoleWidget::refreshPeriodicTxTable()
{
    ui->txPeriodicTable->setRowCount(0);

    for (const ConsoleTxMessageConfig& config : m_periodicTxMessages)
    {
        const int row = ui->txPeriodicTable->rowCount();
        ui->txPeriodicTable->insertRow(row);
        ui->txPeriodicTable->setItem(row, 0, new QTableWidgetItem(config.deviceName));
        ui->txPeriodicTable->setItem(row, 1, new QTableWidgetItem(config.messageName));
        ui->txPeriodicTable->setItem(
            row, 2, new QTableWidgetItem(QString("%1 ms").arg(config.intervalMs)));
        ui->txPeriodicTable->setItem(
            row, 3, new QTableWidgetItem(config.periodicEnabled ? "Running" : "Stopped"));
    }
}

void ConsoleWidget::refreshFilterDropdownOptions()
{
    ui->filterValueComboBox->clear();

    const QString type = ui->filterTypeComboBox->currentText();

    if (type == "Device")
    {
        if (m_pcpDatabase)
        {
            for (uint32_t deviceId : m_pcpDatabase->deviceIds())
                ui->filterValueComboBox->addItem(deviceDisplayName(deviceId));
        }
    }
    else if (type == "Message")
    {
        QSet<QString> messageNames;

        if (m_pcpDatabase)
        {
            for (uint32_t deviceId : m_pcpDatabase->deviceIds())
            {
                auto devOpt = m_pcpDatabase->device(deviceId);
                if (!devOpt.has_value())
                    continue;

                for (const auto& [messageName, _] : devOpt->messagesByName)
                    messageNames.insert(QString::fromStdString(messageName));
            }
        }

        for (const QString& name : messageNames)
            ui->filterValueComboBox->addItem(name);
    }
}

void ConsoleWidget::refreshFilterValueWidget()
{
    const QString type = ui->filterTypeComboBox->currentText();
    const bool useCombo = (type == "Device" || type == "Message");

    ui->filterValueEdit->setVisible(!useCombo);
    ui->filterValueComboBox->setVisible(useCombo);

    refreshFilterDropdownOptions();
}

std::optional<ConsoleMessageRecord> ConsoleWidget::parseMessageRecord(const QString& message) const
{
    QRegularExpression re(
        R"(^\[(.*?)\]\s+(RX|TX)\s+\|\s+([^|]+?)\s+\|\s+([^|]+?)\s+\|\s+Message\s+(\d+)\s+\|\s+DLC\s+(\d+)\s+\|\s+(.*)$)");

    QRegularExpressionMatch match = re.match(message);
    if (!match.hasMatch())
        return std::nullopt;

    ConsoleMessageRecord record;
    record.timestamp = match.captured(1).trimmed();
    record.direction = match.captured(2).trimmed();
    record.deviceName = match.captured(3).trimmed();
    record.messageName = match.captured(4).trimmed();
    record.messageId = match.captured(5).trimmed();
    record.dlc = match.captured(6).trimmed();
    record.data = match.captured(7).trimmed();
    record.rawLine = message;

    return record;
}

void ConsoleWidget::appendMessage(const QString& message)
{
    m_allMessages.append(message);

    const auto parsed = parseMessageRecord(message);
    if (!parsed.has_value())
        return;

    m_messageRecords.append(*parsed);
    enforceVisibleMessageCapacity();

    if (m_paused)
    {
        m_pausedBuffer.append(*parsed);
        enforceVisibleMessageCapacity();
        return;
    }

    m_messageModel->setRecords(m_messageRecords);

    updateMessageTableLayout();
}

void ConsoleWidget::onPauseToggled(bool paused)
{
    m_paused = paused;
    ui->pauseButton->setText(paused ? "Resume" : "Pause");

    if (!m_paused && !m_pausedBuffer.isEmpty())
    {
        m_pausedBuffer.clear();
        enforceVisibleMessageCapacity();
        m_messageModel->setRecords(m_messageRecords);
        refreshView();
    }
}

void ConsoleWidget::onClearClicked()
{
    m_allMessages.clear();
    m_messageRecords.clear();
    m_pausedBuffer.clear();
    m_messageModel->clear();
    updateMessageTableLayout();
}

void ConsoleWidget::onAddSignalClicked()
{
    const QString deviceName = ui->deviceComboBox->currentText().trimmed();
    const QString messageName = ui->messageComboBox->currentText().trimmed();
    const QString signalName = ui->signalComboBox->currentText().trimmed();

    if (deviceName.isEmpty() || messageName.isEmpty() || signalName.isEmpty())
        return;

    addWatchedSignal(deviceName, messageName, signalName);
    clearSignalSelections();
}

void ConsoleWidget::onRemoveSignalClicked()
{
    removeSelectedSignal();
    clearSignalSelections();
}

void ConsoleWidget::onClearSignalSelectionClicked()
{
    ui->signalsTable->clearSelection();
    ui->deviceComboBox->setCurrentIndex(0);
    ui->messageComboBox->clear();
    ui->messageComboBox->setCurrentIndex(-1);
    ui->signalComboBox->clear();
    ui->signalComboBox->setCurrentIndex(-1);
    clearSignalSelections();
}

void ConsoleWidget::onStatusMessage(const QString& message)
{
    processDecodedStatusBlock(message);
}

void ConsoleWidget::onDeviceChanged(int)
{
    refreshMessageCombo();
    refreshSignalCombo();
}

void ConsoleWidget::onMessageChanged(int)
{
    refreshSignalCombo();
}

void ConsoleWidget::onAddFilterClicked()
{
    const QString type = ui->filterTypeComboBox->currentText().trimmed();
    QString value;

    if (type == "Device" || type == "Message")
        value = ui->filterValueComboBox->currentText().trimmed();
    else
        value = ui->filterValueEdit->text().trimmed();

    if (type.isEmpty() || value.isEmpty())
        return;

    for (int i = 0; i < ui->filtersTable->rowCount(); ++i)
    {
        const QString existingType = ui->filtersTable->item(i, 0)->text();
        const QString existingValue = ui->filtersTable->item(i, 1)->text();
        if (existingType == type && existingValue.compare(value, Qt::CaseInsensitive) == 0)
            return;
    }

    const int row = ui->filtersTable->rowCount();
    ui->filtersTable->insertRow(row);
    ui->filtersTable->setItem(row, 0, new QTableWidgetItem(type));
    ui->filtersTable->setItem(row, 1, new QTableWidgetItem(value));

    ui->filterValueEdit->clear();

    rebuildFilterSetsFromTable();
    refreshView();
    clearFilterSelections();
}

void ConsoleWidget::onRemoveFilterClicked()
{
    const int row = ui->filtersTable->currentRow();
    if (row < 0)
        return;

    ui->filtersTable->removeRow(row);
    ui->filtersTable->clearSelection();

    rebuildFilterSetsFromTable();
    refreshView();
    clearFilterSelections();
}

void ConsoleWidget::onFilterTypeChanged(int)
{
    refreshFilterValueWidget();
}

void ConsoleWidget::onTxDeviceChanged(int)
{
    refreshTxMessageCombo();
    refreshTxSignalTable();
}

void ConsoleWidget::onTxMessageChanged(int)
{
    refreshTxSignalTable();
    ui->txMessageComboBox->clearFocus();
}

void ConsoleWidget::onSendTxClicked()
{
    sendSelectedTxMessage();
}

void ConsoleWidget::onAddPeriodicTxClicked()
{
    ConsoleTxMessageConfig config;
    if (!buildSelectedTxMessage(config))
        return;

    const QString key = makePeriodicTxKey(config.deviceId, config.messageName);
    for (int i = 0; i < m_periodicTxMessages.size(); ++i)
    {
        if (makePeriodicTxKey(m_periodicTxMessages[i].deviceId,
                              m_periodicTxMessages[i].messageName) == key)
        {
            config.periodicEnabled = m_periodicTxMessages[i].periodicEnabled;
            config.elapsedMs = 0;
            m_periodicTxMessages[i] = config;
            refreshPeriodicTxTable();
            ui->txStatusLabel->setText(
                QString("Updated periodic message: %1 | %2")
                    .arg(config.deviceName)
                    .arg(config.messageName));
            syncPeriodicTimerState();
            clearSendSelections();
            return;
        }
    }

    m_periodicTxMessages.append(config);
    refreshPeriodicTxTable();
    ui->txStatusLabel->setText(
        QString("Added periodic message: %1 | %2")
            .arg(config.deviceName)
            .arg(config.messageName));
    syncPeriodicTimerState();
    clearSendSelections();
}

void ConsoleWidget::onRemovePeriodicTxClicked()
{
    const int row = ui->txPeriodicTable->currentRow();
    if (row < 0 || row >= m_periodicTxMessages.size())
        return;

    const ConsoleTxMessageConfig removed = m_periodicTxMessages.takeAt(row);
    refreshPeriodicTxTable();
    ui->txPeriodicTable->clearSelection();
    ui->txStatusLabel->setText(
        QString("Removed periodic message: %1 | %2")
            .arg(removed.deviceName)
            .arg(removed.messageName));
    syncPeriodicTimerState();
    clearSendSelections();
}

void ConsoleWidget::onPeriodicTxRowDoubleClicked(int row, int column)
{
    Q_UNUSED(column);

    if (row < 0 || row >= m_periodicTxMessages.size())
        return;

    const ConsoleTxMessageConfig& config = m_periodicTxMessages[row];
    const int deviceIndex = ui->txDeviceComboBox->findText(config.deviceName);
    if (deviceIndex >= 0)
        ui->txDeviceComboBox->setCurrentIndex(deviceIndex);

    refreshTxMessageCombo();

    const int messageIndex = ui->txMessageComboBox->findText(config.messageName);
    if (messageIndex >= 0)
        ui->txMessageComboBox->setCurrentIndex(messageIndex);

    refreshTxSignalTable();
    ui->txIntervalSpinBox->setValue(config.intervalMs);

    for (int i = 0; i < ui->txSignalsTable->rowCount(); ++i)
    {
        QTableWidgetItem* signalItem = ui->txSignalsTable->item(i, 0);
        QTableWidgetItem* valueItem = ui->txSignalsTable->item(i, 1);
        if (!signalItem || !valueItem)
            continue;

        const auto it = config.signalValues.constFind(signalItem->text());
        if (it != config.signalValues.cend())
            valueItem->setText(QString::number(it.value()));
    }

    ui->txStatusLabel->setText(
        QString("Loaded periodic message: %1 | %2")
            .arg(config.deviceName)
            .arg(config.messageName));
    clearSendSelections();
}

void ConsoleWidget::onStartPeriodicTxClicked()
{
    const int row = ui->txPeriodicTable->currentRow();
    if (row < 0 || row >= m_periodicTxMessages.size())
        return;

    m_periodicTxMessages[row].periodicEnabled = true;
    m_periodicTxMessages[row].elapsedMs = 0;
    refreshPeriodicTxTable();
    ui->txPeriodicTable->selectRow(row);
    syncPeriodicTimerState();
    ui->txStatusLabel->setText(
        QString("Started periodic send: %1 | %2")
            .arg(m_periodicTxMessages[row].deviceName)
            .arg(m_periodicTxMessages[row].messageName));
    clearSendSelections();
}

void ConsoleWidget::onStopPeriodicTxClicked()
{
    const int row = ui->txPeriodicTable->currentRow();
    if (row < 0 || row >= m_periodicTxMessages.size())
        return;

    m_periodicTxMessages[row].periodicEnabled = false;
    m_periodicTxMessages[row].elapsedMs = 0;
    refreshPeriodicTxTable();
    ui->txPeriodicTable->selectRow(row);
    syncPeriodicTimerState();
    ui->txStatusLabel->setText(
        QString("Stopped periodic send: %1 | %2")
            .arg(m_periodicTxMessages[row].deviceName)
            .arg(m_periodicTxMessages[row].messageName));
    clearSendSelections();
}

void ConsoleWidget::onTxPeriodChanged(int intervalMs)
{
    Q_UNUSED(intervalMs);
}

void ConsoleWidget::onTxTimerTimeout()
{
    const int tickMs = m_txTimer.interval();

    for (ConsoleTxMessageConfig& config : m_periodicTxMessages)
    {
        if (!config.periodicEnabled)
            continue;

        config.elapsedMs += tickMs;
        if (config.elapsedMs < config.intervalMs)
            continue;

        config.elapsedMs = 0;
        if (!sendTxMessage(config))
            return;
    }
}

QString ConsoleWidget::extractDeviceName(const QString& message) const
{
    const auto parsed = parseMessageRecord(message);
    return parsed.has_value() ? parsed->deviceName : QString();
}

QString ConsoleWidget::extractMessageId(const QString& message) const
{
    const auto parsed = parseMessageRecord(message);
    return parsed.has_value() ? parsed->messageId : QString();
}

QString ConsoleWidget::extractMessageName(const QString& message) const
{
    const auto parsed = parseMessageRecord(message);
    return parsed.has_value() ? parsed->messageName : QString();
}

void ConsoleWidget::refreshView()
{
    m_messageProxyModel->setTextFilters(m_textFilters);
    m_messageProxyModel->setDeviceFilters(m_deviceFilters);
    m_messageProxyModel->setMessageIdFilters(m_messageIdFilters);
    m_messageProxyModel->setMessageNameFilters(m_messageNameFilters);

    updateMessageTableLayout();
}

void ConsoleWidget::enforceVisibleMessageCapacity()
{
    const int maxRows = visibleMessageCapacity();
    if (maxRows <= 0)
        return;

    while (m_messageRecords.size() > maxRows)
        m_messageRecords.removeFirst();

    while (m_pausedBuffer.size() > maxRows)
        m_pausedBuffer.removeFirst();

    while (m_allMessages.size() > maxRows)
        m_allMessages.removeFirst();
}

int ConsoleWidget::visibleMessageCapacity() const
{
    if (!ui->consoleTableView)
        return 1;

    const int rowHeight = ui->consoleTableView->verticalHeader()->defaultSectionSize();
    const int viewportHeight = ui->consoleTableView->viewport()->height();

    if (rowHeight <= 0 || viewportHeight <= 0)
        return 1;

    return std::max(1, viewportHeight / rowHeight);
}

void ConsoleWidget::clearSendSelections()
{
    ui->txPeriodicTable->clearSelection();
    ui->txSignalsTable->clearSelection();
    ui->txDeviceComboBox->clearFocus();
    ui->txMessageComboBox->clearFocus();
    ui->sendTxButton->clearFocus();
}

void ConsoleWidget::clearSignalSelections()
{
    ui->signalsTable->clearSelection();
    ui->deviceComboBox->clearFocus();
    ui->messageComboBox->clearFocus();
    ui->signalComboBox->clearFocus();
    ui->addSignalButton->clearFocus();
    ui->removeSignalButton->clearFocus();
    ui->clearSignalSelectionButton->clearFocus();
}

void ConsoleWidget::clearFilterSelections()
{
    ui->filtersTable->clearSelection();
    ui->filterTypeComboBox->clearFocus();
    ui->filterValueEdit->clearFocus();
    ui->filterValueComboBox->clearFocus();
    ui->addFilterButton->clearFocus();
    ui->removeFilterButton->clearFocus();
}

bool ConsoleWidget::sendSelectedTxMessage()
{
    ConsoleTxMessageConfig config;
    if (!buildSelectedTxMessage(config))
        return false;

    const bool sent = sendTxMessage(config);
    clearSendSelections();
    return sent;
}

bool ConsoleWidget::buildSelectedTxMessage(ConsoleTxMessageConfig& config)
{
    if (!m_pcpDatabase)
    {
        ui->txStatusLabel->setText("No PCP database loaded");
        return false;
    }

    const QString deviceText = ui->txDeviceComboBox->currentText().trimmed();
    const QString messageText = ui->txMessageComboBox->currentText().trimmed();
    const auto deviceIdOpt = deviceIdFromDisplayName(deviceText);

    if (!deviceIdOpt.has_value() || messageText.isEmpty())
    {
        ui->txStatusLabel->setText("Select a device and message");
        return false;
    }

    const PCPMessageDefinition* msgDef =
        m_pcpDatabase->messageByName(*deviceIdOpt, messageText.toStdString());

    if (!msgDef)
    {
        ui->txStatusLabel->setText("Selected message was not found");
        return false;
    }

    config.deviceName = deviceText;
    config.deviceId = *deviceIdOpt;
    config.messageName = messageText;
    config.intervalMs = ui->txIntervalSpinBox->value();
    config.signalValues.clear();

    for (int row = 0; row < ui->txSignalsTable->rowCount(); ++row)
    {
        QTableWidgetItem* signalItem = ui->txSignalsTable->item(row, 0);
        QTableWidgetItem* valueItem = ui->txSignalsTable->item(row, 1);

        if (!signalItem || !valueItem)
            continue;

        bool ok = false;
        const double value = valueItem->text().trimmed().toDouble(&ok);
        if (!ok)
        {
            ui->txStatusLabel->setText(
                QString("Invalid value for signal %1").arg(signalItem->text()));
            return false;
        }

        config.signalValues.insert(signalItem->text(), value);
    }

    return true;
}

bool ConsoleWidget::sendTxMessage(const ConsoleTxMessageConfig& config)
{
    try
    {
        std::map<std::string, double> signalValues;
        for (auto it = config.signalValues.cbegin(); it != config.signalValues.cend(); ++it)
            signalValues[it.key().toStdString()] = it.value();

        const PCPFrame frame =
            m_txEncoder.encode(config.deviceId, config.messageName.toStdString(), signalValues);

        Logger::instance().logComs(
            PCPFormatter::toConsoleString(frame, "TX", *m_pcpDatabase));

        const auto decoded = m_txDecoder.decode(frame.id, frame.dlc, frame.data);
        if (decoded.has_value())
            Logger::instance().logDecoded(PCPDecodeFormatter::toText(decoded.value()));
    }
    catch (const std::exception&)
    {
        ui->txStatusLabel->setText("Failed to encode TX frame");
        return false;
    }

    const QString modeText =
        config.periodicEnabled ? "Periodic message sent" : "Message sent";

    ui->txStatusLabel->setText(
        QString("%1: %2 | %3")
            .arg(modeText)
            .arg(config.deviceName)
            .arg(config.messageName));

    return true;
}

QString ConsoleWidget::makePeriodicTxKey(uint32_t deviceId, const QString& messageName) const
{
    return QString("%1::%2").arg(deviceId).arg(messageName);
}

void ConsoleWidget::syncPeriodicTimerState()
{
    bool hasActiveMessages = false;

    for (const ConsoleTxMessageConfig& config : m_periodicTxMessages)
    {
        if (!config.periodicEnabled)
            continue;

        hasActiveMessages = true;
        break;
    }

    if (hasActiveMessages)
        m_txTimer.start();
    else
        m_txTimer.stop();
}

bool ConsoleWidget::eventFilter(QObject *watched, QEvent *event)
{
    auto clearSelectionOnRepeatClick = [&](QTableWidget* table) -> bool
    {
        if (watched != table->viewport() || event->type() != QEvent::MouseButtonPress)
            return false;

        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        const QModelIndex index = table->indexAt(mouseEvent->pos());
        if (!index.isValid())
        {
            table->clearSelection();
            table->clearFocus();
            return false;
        }

        if (table->currentRow() == index.row() && table->selectionModel() &&
            table->selectionModel()->isRowSelected(index.row(), QModelIndex()))
        {
            table->clearSelection();
            table->clearFocus();
            return true;
        }

        return false;
    };

    if (clearSelectionOnRepeatClick(ui->signalsTable))
        return true;

    if (clearSelectionOnRepeatClick(ui->filtersTable))
        return true;

    if (clearSelectionOnRepeatClick(ui->txPeriodicTable))
        return true;

    return QWidget::eventFilter(watched, event);
}

void ConsoleWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    enforceVisibleMessageCapacity();
    m_messageModel->setRecords(m_messageRecords);
    updateMessageTableLayout();
}

QString ConsoleWidget::makeWatchKey(const QString& deviceName,
                                    const QString& messageName,
                                    const QString& signalName) const
{
    return deviceName + "::" + messageName + "::" + signalName;
}

void ConsoleWidget::addWatchedSignal(const QString& deviceName,
                                     const QString& messageName,
                                     const QString& signalName)
{
    const QString key = makeWatchKey(deviceName, messageName, signalName);

    if (m_watchRowByKey.contains(key))
        return;

    const int row = ui->signalsTable->rowCount();
    ui->signalsTable->insertRow(row);

    ui->signalsTable->setItem(row, 0, new QTableWidgetItem(deviceName));
    ui->signalsTable->setItem(row, 1, new QTableWidgetItem(signalName));
    ui->signalsTable->setItem(row, 2, new QTableWidgetItem(messageName));
    ui->signalsTable->setItem(row, 3, new QTableWidgetItem("-"));

    m_watchRowByKey.insert(key, row);
}

void ConsoleWidget::removeSelectedSignal()
{
    const int row = ui->signalsTable->currentRow();
    if (row < 0)
        return;

    QTableWidgetItem *deviceItem = ui->signalsTable->item(row, 0);
    QTableWidgetItem *signalItem = ui->signalsTable->item(row, 1);
    QTableWidgetItem *messageItem = ui->signalsTable->item(row, 2);

    if (!deviceItem || !signalItem || !messageItem)
        return;

    const QString key = makeWatchKey(deviceItem->text(), messageItem->text(), signalItem->text());

    ui->signalsTable->removeRow(row);
    m_watchRowByKey.remove(key);

    for (int i = 0; i < ui->signalsTable->rowCount(); ++i)
    {
        const QString deviceName = ui->signalsTable->item(i, 0)->text();
        const QString signalName = ui->signalsTable->item(i, 1)->text();
        const QString messageName = ui->signalsTable->item(i, 2)->text();

        m_watchRowByKey[makeWatchKey(deviceName, messageName, signalName)] = i;
    }

    ui->signalsTable->clearSelection();
}

void ConsoleWidget::updateSignalValue(const QString& deviceName,
                                      const QString& messageName,
                                      const QString& signalName,
                                      const QString& value)
{
    const QString key = makeWatchKey(deviceName, messageName, signalName);

    if (!m_watchRowByKey.contains(key))
        return;

    const int row = m_watchRowByKey.value(key);
    QTableWidgetItem *valueItem = ui->signalsTable->item(row, 3);

    if (!valueItem)
    {
        valueItem = new QTableWidgetItem;
        ui->signalsTable->setItem(row, 3, valueItem);
    }

    valueItem->setText(value);
}

void ConsoleWidget::processDecodedStatusBlock(const QString& message)
{
    const QStringList lines = message.split('\n', Qt::SkipEmptyParts);

    QString deviceName;
    QString messageName;
    bool hasHeader = false;

    QRegularExpression headerRe(
        R"(^(?:\[[^\]]+\]\s+)?PCP\s+([A-Za-z_][A-Za-z0-9_]*)\s+\|\s+DEV\s*=\s*([0-9]+)\s+\|\s+NAME\s*=\s*([^|]+?)\s+\|\s+MSG\s*=\s*([0-9]+))");

    QRegularExpression signalRe(
        R"(^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*([^\(]+))");

    for (const QString &line : lines)
    {
        QRegularExpressionMatch headerMatch = headerRe.match(line);
        if (headerMatch.hasMatch())
        {
            messageName = headerMatch.captured(1).trimmed();

            const int deviceId = headerMatch.captured(2).toInt();
            const QString resolvedName = headerMatch.captured(3).trimmed();

            deviceName = QString("%1 (%2)").arg(resolvedName).arg(deviceId);
            hasHeader = true;
            continue;
        }

        QRegularExpressionMatch signalMatch = signalRe.match(line);
        if (!signalMatch.hasMatch() || !hasHeader)
            continue;

        const QString signalName = signalMatch.captured(1).trimmed();
        const QString value = signalMatch.captured(2).trimmed();

        updateSignalValue(deviceName, messageName, signalName, value);
    }
}

void ConsoleWidget::rebuildFilterSetsFromTable()
{
    m_textFilters.clear();
    m_deviceFilters.clear();
    m_messageIdFilters.clear();
    m_messageNameFilters.clear();

    for (int i = 0; i < ui->filtersTable->rowCount(); ++i)
    {
        QTableWidgetItem *typeItem = ui->filtersTable->item(i, 0);
        QTableWidgetItem *valueItem = ui->filtersTable->item(i, 1);

        if (!typeItem || !valueItem)
            continue;

        const QString type = typeItem->text().trimmed();
        const QString value = valueItem->text().trimmed();

        if (value.isEmpty())
            continue;

        if (type == "Text")
            m_textFilters.insert(value);
        else if (type == "Device")
            m_deviceFilters.insert(value);
        else if (type == "Message ID")
            m_messageIdFilters.insert(value);
        else if (type == "Message")
            m_messageNameFilters.insert(value);
    }
}

void ConsoleWidget::updateMessageTableLayout()
{
    if (!ui->consoleTableView || !ui->consoleTableView->model())
        return;

    auto *header = ui->consoleTableView->horizontalHeader();

    header->setStretchLastSection(false);

    header->setSectionResizeMode(0, QHeaderView::ResizeToContents); // Time
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents); // Dir
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents); // Device
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents); // Name
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents); // Msg
    header->setSectionResizeMode(5, QHeaderView::ResizeToContents); // DLC
    header->setSectionResizeMode(6, QHeaderView::Interactive);      // Data

    ui->consoleTableView->resizeColumnToContents(0);
    ui->consoleTableView->resizeColumnToContents(1);
    ui->consoleTableView->resizeColumnToContents(2);
    ui->consoleTableView->resizeColumnToContents(3);
    ui->consoleTableView->resizeColumnToContents(4);
    ui->consoleTableView->resizeColumnToContents(5);
    ui->consoleTableView->resizeColumnToContents(6);

    const int viewportWidth = ui->consoleTableView->viewport()->width();

    int usedWidth = 0;
    for (int col = 0; col < 6; ++col)
        usedWidth += ui->consoleTableView->columnWidth(col);

    const int remaining = viewportWidth - usedWidth - 24;

    if (remaining > ui->consoleTableView->columnWidth(6))
        ui->consoleTableView->setColumnWidth(6, remaining);
}
