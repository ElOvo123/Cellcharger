#pragma once

#include <QAbstractTableModel>
#include <QList>
#include <QString>

struct ConsoleMessageRecord
{
    QString timestamp;
    QString direction;
    QString deviceName;
    QString messageName;
    QString messageId;
    QString dlc;
    QString data;
    QString rawLine;
};

class ConsoleMessageModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit ConsoleMessageModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    void clear();
    void setRecords(const QList<ConsoleMessageRecord>& records);
    void appendRecord(const ConsoleMessageRecord& record);

    const ConsoleMessageRecord& recordAt(int row) const;

private:
    QList<ConsoleMessageRecord> m_records;

    static bool lessThan(const ConsoleMessageRecord& a, const ConsoleMessageRecord& b, int column, Qt::SortOrder order);
};