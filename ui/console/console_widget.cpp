#include "console_widget.h"
#include "ui_console_widget.h"
#include "logger_backend.h"

#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>

ConsoleWidget::ConsoleWidget(QWidget *parent) : QWidget(parent), ui(new Ui::ConsoleWidget)
{
    ui->setupUi(this);

    ui->closeButton->setCursor(Qt::PointingHandCursor);

    ui->closeButton->setStyleSheet("QPushButton { border:none; font-weight:bold; font-size:16px; }" "QPushButton:hover { background:#d9d9d9; }");
    ui->pauseButton->setStyleSheet("QPushButton { padding: 2px 8px; }" "QPushButton:checked { background:#d9d9d9; }");

    for (const QString &line : Logger::instance().history()) 
    {
        m_allMessages.append(line);
    }
    
    refreshView();

    connect(&Logger::instance(), &Logger::newLogMessage, this, &ConsoleWidget::appendMessage, Qt::QueuedConnection);
    connect(ui->closeButton, &QPushButton::clicked, this, [this]() {emit closeRequested(this);});
    connect(ui->pauseButton, &QPushButton::toggled, this, [this](bool checked) { m_paused = checked; ui->pauseButton->setText(checked ? "Resume" : "Pause");});
    connect(ui->clearButton, &QPushButton::clicked, this, &ConsoleWidget::onClearClicked);
    connect(ui->filterEdit, &QLineEdit::textChanged, this, &ConsoleWidget::onFilterTextChanged);
}

ConsoleWidget::~ConsoleWidget()
{
    delete ui;
}

void ConsoleWidget::appendMessage(const QString& message)
{
    m_allMessages.append(message);

    if (m_paused) 
    {
        return;
    }

    if (passesFilter(message)) 
    {
        ui->consoleOutput->append(message);
    }
}

void ConsoleWidget::onFilterTextChanged(const QString&)
{
    refreshView();
}

void ConsoleWidget::onClearClicked()
{
    m_allMessages.clear();
    ui->consoleOutput->clear();
}

bool ConsoleWidget::passesFilter(const QString& message) const
{
    const QString filter = ui->filterEdit->text().trimmed();
    
    if (filter.isEmpty()) 
    {
        return true;
    }

    return message.contains(filter, Qt::CaseInsensitive);
}

void ConsoleWidget::refreshView()
{
    ui->consoleOutput->clear();

    for (const QString &line : m_allMessages) 
    {
        if (passesFilter(line)) 
        {
            ui->consoleOutput->append(line);
        }
    }
}