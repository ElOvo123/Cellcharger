#pragma once

#include <QHash>
#include <QMap>
#include <QStringList>
#include <QWidget>

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

private slots:
    void onFilterTextChanged(const QString& text);
    void onIdFilterTextChanged(const QString& text);
    void onClearClicked();
    void onPauseToggled(bool paused);
    void onAddSignalClicked();
    void onRemoveSignalClicked();
    void onStatusMessage(const QString& message);
    void onDeviceChanged(int index);

private:
    Ui::ConsoleWidget *ui;

    QStringList m_allMessages;
    bool m_paused = false;

    QMap<QString, QStringList> m_availableSignalsByDevice;
    QHash<QString, int> m_watchRowByKey;

    void setupSignalSelectors();
    void refreshSignalCombo();
    void refreshView();
    bool passesFilter(const QString& message) const;
    bool passesTextFilter(const QString& message) const;
    bool passesIdFilter(const QString& message) const;

    void addWatchedSignal(const QString& deviceName, const QString& signalName);
    void removeSelectedSignal();
    void updateSignalValue(const QString& deviceName, const QString& signalName, const QString& value);
    void processDecodedStatusBlock(const QString& message);
    QString makeWatchKey(const QString& deviceName, const QString& signalName) const;
};