#pragma once

#include <QHash>
#include <QList>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <QWidget>
#include <optional>

#include "pcp_database.h"
#include "pcp_decoder.h"
#include "pcp_encoder.h"
#include "console_message_model.h"
#include "console_message_filter_proxy_model.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
class ConsoleWidget;
}
QT_END_NAMESPACE

struct ConsoleTxMessageConfig
{
    QString deviceName;
    uint32_t deviceId = 0;
    QString messageName;
    QMap<QString, double> signalValues;
    int intervalMs = 1000;
    bool periodicEnabled = false;
    int elapsedMs = 0;
};

class ConsoleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ConsoleWidget(QWidget* parent = nullptr);
    explicit ConsoleWidget(const PCPDatabase* pcpDatabase, QWidget* parent = nullptr);
    ~ConsoleWidget();

    static void setSharedPCPDatabase(const PCPDatabase* pcpDatabase);

public slots:
    void appendMessage(const QString& message);

private slots:
    void onClearClicked();
    void onPauseToggled(bool paused);

    void onAddSignalClicked();
    void onRemoveSignalClicked();
    void onClearSignalSelectionClicked();
    void onStatusMessage(const QString& message);
    void onDeviceChanged(int index);
    void onMessageChanged(int index);

    void onAddFilterClicked();
    void onRemoveFilterClicked();
    void onFilterTypeChanged(int index);
    void onTxDeviceChanged(int index);
    void onTxMessageChanged(int index);
    void onSendTxClicked();
    void onAddPeriodicTxClicked();
    void onStartPeriodicTxClicked();
    void onStopPeriodicTxClicked();
    void onRemovePeriodicTxClicked();
    void onPeriodicTxRowDoubleClicked(int row, int column);
    void onTxPeriodChanged(int intervalMs);
    void onTxTimerTimeout();

private:
    Ui::ConsoleWidget* ui;
    const PCPDatabase* m_pcpDatabase = nullptr;

    static const PCPDatabase* s_sharedPCPDatabase;

    QStringList m_allMessages;
    QList<ConsoleMessageRecord> m_messageRecords;
    QList<ConsoleMessageRecord> m_pausedBuffer;
    ConsoleMessageModel* m_messageModel = nullptr;
    ConsoleMessageFilterProxyModel* m_messageProxyModel = nullptr;
    bool m_paused = false;

    QHash<QString, int> m_watchRowByKey;

    QSet<QString> m_textFilters;
    QSet<QString> m_deviceFilters;
    QSet<QString> m_messageIdFilters;
    QSet<QString> m_messageNameFilters;
    QTimer m_txTimer;
    int m_txCounter = 0;
    PCPEncoder m_txEncoder;
    PCPDecoder m_txDecoder;
    QList<ConsoleTxMessageConfig> m_periodicTxMessages;

    void setupMessageView();
    void setupSignalView();
    void setupFilterView();
    void setupTxView();
    void setupSignalSelectors();

    void refreshMessageCombo();
    void refreshSignalCombo();
    void refreshTxMessageCombo();
    void refreshTxSignalTable();
    void refreshPeriodicTxTable();
    void refreshFilterValueWidget();
    void refreshFilterDropdownOptions();
    void refreshView();
    void enforceVisibleMessageCapacity();
    int visibleMessageCapacity() const;
    void clearSendSelections();
    void clearSignalSelections();
    void clearFilterSelections();
    bool sendSelectedTxMessage();
    bool buildSelectedTxMessage(ConsoleTxMessageConfig& config);
    bool sendTxMessage(const ConsoleTxMessageConfig& config);
    QString makePeriodicTxKey(uint32_t deviceId, const QString& messageName) const;
    void syncPeriodicTimerState();

    std::optional<ConsoleMessageRecord> parseMessageRecord(const QString& message) const;
    QString extractDeviceName(const QString& message) const;
    QString extractMessageId(const QString& message) const;
    QString extractMessageName(const QString& message) const;

    void addWatchedSignal(const QString& deviceName, const QString& messageName, const QString& signalName);
    void removeSelectedSignal();
    void updateSignalValue(const QString& deviceName, const QString& messageName, const QString& signalName,
                           const QString& value);
    void processDecodedStatusBlock(const QString& message);

    QString makeWatchKey(const QString& deviceName, const QString& messageName, const QString& signalName) const;

    void rebuildFilterSetsFromTable();
    void updateMessageTableLayout();

    QString deviceDisplayName(uint32_t deviceId) const;
    std::optional<uint32_t> deviceIdFromDisplayName(const QString& displayName) const;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
};
