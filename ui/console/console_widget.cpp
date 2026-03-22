#include "console_widget.h"
#include "ui_console_widget.h"
#include "logger_backend.h"

#include <QComboBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>

ConsoleWidget::ConsoleWidget(QWidget *parent) : QWidget(parent), ui(new Ui::ConsoleWidget)
{
    ui->setupUi(this);

    ui->pauseButton->setCheckable(true);

    ui->signalsTable->horizontalHeader()->setStretchLastSection(true);
    ui->signalsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->signalsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->signalsTable->verticalHeader()->setVisible(false);

    setupSignalSelectors();

    connect(ui->pauseButton, &QPushButton::toggled, this, &ConsoleWidget::onPauseToggled);
    connect(ui->clearButton, &QPushButton::clicked, this, &ConsoleWidget::onClearClicked);
    connect(ui->filterEdit, &QLineEdit::textChanged, this, &ConsoleWidget::onFilterTextChanged);
    connect(ui->idFilterEdit, &QLineEdit::textChanged, this, &ConsoleWidget::onIdFilterTextChanged);
    connect(ui->addSignalButton, &QPushButton::clicked, this, &ConsoleWidget::onAddSignalClicked);
    connect(ui->removeSignalButton, &QPushButton::clicked, this, &ConsoleWidget::onRemoveSignalClicked);
    connect(ui->deviceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ConsoleWidget::onDeviceChanged);

    for (const QString &line : Logger::instance().comsHistory())
        m_allMessages.append(line);

    refreshView();

    connect(&Logger::instance(),&Logger::newComsMessage, this, &ConsoleWidget::appendMessage, Qt::QueuedConnection);
    connect(&Logger::instance(), &Logger::newDecodedMessage, this, &ConsoleWidget::onStatusMessage, Qt::QueuedConnection);
}

ConsoleWidget::~ConsoleWidget()
{
    delete ui;
}

void ConsoleWidget::setupSignalSelectors()
{
    m_availableSignalsByDevice.clear();

    m_availableSignalsByDevice["Device 1"] = {"voltage", "current", "temperature", "state"};
    m_availableSignalsByDevice["Device 2"] = {"voltage", "current", "temperature", "state"};
    m_availableSignalsByDevice["Device 3"] = {"voltage", "current", "temperature", "state"};

    ui->deviceComboBox->clear();
    ui->deviceComboBox->addItems(m_availableSignalsByDevice.keys());

    refreshSignalCombo();
}

void ConsoleWidget::refreshSignalCombo()
{
    const QString currentSignal = ui->signalComboBox->currentText();
    const QString deviceName = ui->deviceComboBox->currentText();

    ui->signalComboBox->clear();
    ui->signalComboBox->addItems(m_availableSignalsByDevice.value(deviceName));

    const int signalIndex = ui->signalComboBox->findText(currentSignal);
    if (signalIndex >= 0)
        ui->signalComboBox->setCurrentIndex(signalIndex);
}

void ConsoleWidget::appendMessage(const QString& message)
{
    m_allMessages.append(message);

    if (m_paused)
        return;

    if (passesFilter(message))
        ui->consoleOutput->append(message);
}

void ConsoleWidget::onPauseToggled(bool paused)
{
    m_paused = paused;
    ui->pauseButton->setText(paused ? "Resume" : "Pause");
}

void ConsoleWidget::onClearClicked()
{
    m_allMessages.clear();
    ui->consoleOutput->clear();
}

void ConsoleWidget::onFilterTextChanged(const QString&)
{
    refreshView();
}

void ConsoleWidget::onIdFilterTextChanged(const QString&)
{
    refreshView();
}

void ConsoleWidget::onAddSignalClicked()
{
    const QString deviceName = ui->deviceComboBox->currentText().trimmed();
    const QString signalName = ui->signalComboBox->currentText().trimmed();

    if (deviceName.isEmpty() || signalName.isEmpty())
        return;

    addWatchedSignal(deviceName, signalName);
}

void ConsoleWidget::onRemoveSignalClicked()
{
    removeSelectedSignal();
}

void ConsoleWidget::onStatusMessage(const QString& message)
{
    processDecodedStatusBlock(message);
}

void ConsoleWidget::onDeviceChanged(int)
{
    refreshSignalCombo();
}

bool ConsoleWidget::passesFilter(const QString& message) const
{
    return passesTextFilter(message) && passesIdFilter(message);
}

bool ConsoleWidget::passesTextFilter(const QString& message) const
{
    const QString filter = ui->filterEdit->text().trimmed();

    if (filter.isEmpty())
        return true;

    return message.contains(filter, Qt::CaseInsensitive);
}

bool ConsoleWidget::passesIdFilter(const QString& message) const
{
    const QString idFilter = ui->idFilterEdit->text().trimmed();
    if (idFilter.isEmpty())
        return true;

    QRegularExpression re(R"(0[xX]([0-9A-Fa-f]+))");
    QRegularExpressionMatch match = re.match(message);

    if (!match.hasMatch())
        return false;

    const QString messageId = match.captured(1).toUpper();
    return messageId.contains(idFilter.toUpper());
}

void ConsoleWidget::refreshView()
{
    ui->consoleOutput->clear();

    for (const QString &line : m_allMessages)
    {
        if (passesFilter(line))
            ui->consoleOutput->append(line);
    }
}

QString ConsoleWidget::makeWatchKey(const QString& deviceName, const QString& signalName) const
{
    return deviceName + "::" + signalName;
}

void ConsoleWidget::addWatchedSignal(const QString& deviceName, const QString& signalName)
{
    const QString key = makeWatchKey(deviceName, signalName);

    if (m_watchRowByKey.contains(key))
        return;

    const int row = ui->signalsTable->rowCount();
    ui->signalsTable->insertRow(row);

    ui->signalsTable->setItem(row, 0, new QTableWidgetItem(deviceName));
    ui->signalsTable->setItem(row, 1, new QTableWidgetItem(signalName));
    ui->signalsTable->setItem(row, 2, new QTableWidgetItem("-"));

    m_watchRowByKey.insert(key, row);
}

void ConsoleWidget::removeSelectedSignal()
{
    const int row = ui->signalsTable->currentRow();
    if (row < 0)
        return;

    const int currentDeviceIndex = ui->deviceComboBox->currentIndex();
    const QString currentSignal = ui->signalComboBox->currentText();

    QTableWidgetItem *deviceItem = ui->signalsTable->item(row, 0);
    QTableWidgetItem *signalItem = ui->signalsTable->item(row, 1);

    if (!deviceItem || !signalItem)
        return;

    const QString key = makeWatchKey(deviceItem->text(), signalItem->text());

    ui->signalsTable->removeRow(row);
    m_watchRowByKey.remove(key);

    for (int i = 0; i < ui->signalsTable->rowCount(); ++i)
    {
        const QString deviceName = ui->signalsTable->item(i, 0)->text();
        const QString signalName = ui->signalsTable->item(i, 1)->text();
        m_watchRowByKey[makeWatchKey(deviceName, signalName)] = i;
    }

    ui->signalsTable->clearSelection();

    if (currentDeviceIndex >= 0 && currentDeviceIndex < ui->deviceComboBox->count())
        ui->deviceComboBox->setCurrentIndex(currentDeviceIndex);

    refreshSignalCombo();

    const int signalIndex = ui->signalComboBox->findText(currentSignal);
    if (signalIndex >= 0)
        ui->signalComboBox->setCurrentIndex(signalIndex);
}

void ConsoleWidget::updateSignalValue(const QString& deviceName, const QString& signalName, const QString& value)
{
    const QString key = makeWatchKey(deviceName, signalName);

    if (!m_watchRowByKey.contains(key))
        return;

    const int row = m_watchRowByKey.value(key);
    QTableWidgetItem *valueItem = ui->signalsTable->item(row, 2);

    if (!valueItem)
    {
        valueItem = new QTableWidgetItem;
        ui->signalsTable->setItem(row, 2, valueItem);
    }

    valueItem->setText(value);
}

void ConsoleWidget::processDecodedStatusBlock(const QString& message)
{
    const QStringList lines = message.split('\n', Qt::SkipEmptyParts);

    QString deviceName;
    bool hasDevice = false;

    QRegularExpression deviceRe(R"(DEV\s*=\s*([0-9]+))");
    QRegularExpression signalRe(R"(^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*([^\(]+))");

    for (const QString &line : lines)
    {
        QRegularExpressionMatch deviceMatch = deviceRe.match(line);
        if (deviceMatch.hasMatch())
        {
            deviceName = QString("Device %1").arg(deviceMatch.captured(1));
            hasDevice = true;
            continue;
        }

        QRegularExpressionMatch signalMatch = signalRe.match(line);
        if (!signalMatch.hasMatch() || !hasDevice)
            continue;

        const QString signalName = signalMatch.captured(1).trimmed();
        const QString value = signalMatch.captured(2).trimmed();

        updateSignalValue(deviceName, signalName, value);
    }
}