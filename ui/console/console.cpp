#include "console.h"
#include <QTextEdit>
#include <QVBoxLayout>

#include "logger_backend.h"

ConsoleWidget::ConsoleWidget(QWidget *parent) : QWidget(parent), m_output(new QTextEdit(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);

    m_output->setReadOnly(true);
    layout->addWidget(m_output);

    for (const QString &line : Logger::instance().history())
        m_output->append(line);

    connect(&Logger::instance(), &Logger::newLogMessage, this, &ConsoleWidget::appendMessage, Qt::QueuedConnection);
}

void ConsoleWidget::appendMessage(const QString& msg)
{
    m_output->append(msg);
}