#include "log_widget.h"
#include "ui_log_widget.h"
#include "logger_backend.h"

#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>

LogWidget::LogWidget(QWidget* parent) : QWidget(parent), ui(new Ui::LogWidget)
{
    ui->setupUi(this);

    ui->pauseButton->setCheckable(true);

    connect(ui->pauseButton, &QPushButton::toggled, this, &LogWidget::onPauseToggled);
    connect(ui->clearButton, &QPushButton::clicked, this, &LogWidget::onClearClicked);
    connect(ui->filterEdit, &QLineEdit::textChanged, this, &LogWidget::onFilterTextChanged);

    for (const QString& line : Logger::instance().statusHistory())
    {
        m_allMessages.append(line);
    }

    refreshView();

    connect(&Logger::instance(), &Logger::newStatusMessage, this, &LogWidget::appendMessage, Qt::QueuedConnection);
}

LogWidget::~LogWidget()
{
    delete ui;
}

void LogWidget::appendMessage(const QString& message)
{
    m_allMessages.append(message);

    if (m_paused)
        return;

    if (passesFilter(message))
        ui->logOutput->append(message);
}

void LogWidget::onPauseToggled(bool paused)
{
    m_paused = paused;

    if (paused)
        ui->pauseButton->setText("Resume");
    else
        ui->pauseButton->setText("Pause");
}

void LogWidget::onClearClicked()
{
    m_allMessages.clear();
    ui->logOutput->clear();
}

void LogWidget::onFilterTextChanged(const QString&)
{
    refreshView();
}

bool LogWidget::passesFilter(const QString& message) const
{
    const QString filter = ui->filterEdit->text().trimmed();

    if (filter.isEmpty())
        return true;

    return message.contains(filter, Qt::CaseInsensitive);
}

void LogWidget::refreshView()
{
    ui->logOutput->clear();

    for (const QString& line : m_allMessages)
    {
        if (passesFilter(line))
            ui->logOutput->append(line);
    }
}