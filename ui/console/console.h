#pragma once

#include <QWidget>

class QTextEdit;

class ConsoleWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConsoleWidget(QWidget *parent = nullptr);

public slots:
    void appendMessage(const QString& msg);

private:
    QTextEdit *m_output;
};