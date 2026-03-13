#pragma once

#include <QWidget>
#include <QStringList>

QT_BEGIN_NAMESPACE
namespace Ui { class LogWidget; }
QT_END_NAMESPACE

class LogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogWidget(QWidget *parent = nullptr);
    ~LogWidget();

public slots:
    void appendMessage(const QString& message);

private slots:
    void onFilterTextChanged(const QString& text);
    void onClearClicked();
    void onPauseToggled(bool paused);

private:
    Ui::LogWidget *ui;

    QStringList m_allMessages;
    bool m_paused = false;

    void refreshView();
    bool passesFilter(const QString& message) const;
};