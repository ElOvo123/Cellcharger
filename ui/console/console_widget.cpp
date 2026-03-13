#include "console_widget.h"
#include "ui_console_widget.h"
#include "logger_backend.h"

#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>

ConsoleWidget::ConsoleWidget(QWidget *parent) : QWidget(parent), ui(new Ui::ConsoleWidget)
{
    ui->setupUi(this);

    ui->pauseButton->setCheckable(true);

    connect(ui->pauseButton, &QPushButton::toggled, this, &ConsoleWidget::onPauseToggled);
    connect(ui->clearButton, &QPushButton::clicked, this, &ConsoleWidget::onClearClicked);
    connect(ui->filterEdit, &QLineEdit::textChanged, this, &ConsoleWidget::onFilterTextChanged);

    for (const QString &line : Logger::instance().comsHistory())
    {
        m_allMessages.append(line);
    }

    refreshView();

    connect(&Logger::instance(), &Logger::newComsMessage, this, &ConsoleWidget::appendMessage, Qt::QueuedConnection);
}

ConsoleWidget::~ConsoleWidget()
{
    delete ui;
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

    if (paused)
        ui->pauseButton->setText("Resume");
    else
        ui->pauseButton->setText("Pause");
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

bool ConsoleWidget::passesFilter(const QString& message) const
{
    const QString filter = ui->filterEdit->text().trimmed();

    if (filter.isEmpty())
        return true;

    return message.contains(filter, Qt::CaseInsensitive);
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