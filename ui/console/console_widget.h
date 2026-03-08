#pragma once

#include <QWidget>
#include <QStringList>

QT_BEGIN_NAMESPACE
namespace Ui { class ConsoleWidget; }
QT_END_NAMESPACE

class ConsoleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ConsoleWidget(QWidget *parent = nullptr);
    ~ConsoleWidget();

public slots:
    void appendMessage(const QString& message);

signals:
    void closeRequested(ConsoleWidget *self);

private slots:
    void onFilterTextChanged(const QString& text);
    void onClearClicked();

private:
    Ui::ConsoleWidget *ui;

    QStringList m_allMessages;
    bool m_paused = false;

    void refreshView();
    bool passesFilter(const QString& message) const;
};