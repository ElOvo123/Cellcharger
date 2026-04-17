#include "console_message_model.h"

#include <algorithm>

ConsoleMessageModel::ConsoleMessageModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int ConsoleMessageModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_records.size();
}

int ConsoleMessageModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 7;
}

QVariant ConsoleMessageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() < 0 || index.row() >= m_records.size())
        return QVariant();

    const ConsoleMessageRecord& record = m_records.at(index.row());

    if (role == Qt::DisplayRole)
    {
        switch (index.column())
        {
            case 0: return record.timestamp;
            case 1: return record.direction;
            case 2: return record.deviceName;
            case 3: return record.messageName;
            case 4: return record.messageId;
            case 5: return record.dlc;
            case 6: return record.data;
            default: return QVariant();
        }
    }

    if (role == Qt::TextAlignmentRole)
    {
        switch (index.column())
        {
            case 1:
            case 4:
            case 5:
                return static_cast<int>(Qt::AlignCenter);
            default:
                return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
        }
    }

    return QVariant();
}

QVariant ConsoleMessageModel::headerData(int section,
                                         Qt::Orientation orientation,
                                         int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
            case 0: return "Time";
            case 1: return "Dir";
            case 2: return "Device";
            case 3: return "Name";
            case 4: return "Msg";
            case 5: return "DLC";
            case 6: return "Data";
            default: return QVariant();
        }
    }

    return QVariant();
}

Qt::ItemFlags ConsoleMessageModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void ConsoleMessageModel::clear()
{
    beginResetModel();
    m_records.clear();
    endResetModel();
}

void ConsoleMessageModel::setRecords(const QList<ConsoleMessageRecord>& records)
{
    beginResetModel();
    m_records = records;
    endResetModel();
}

void ConsoleMessageModel::appendRecord(const ConsoleMessageRecord& record)
{
    const int newRow = m_records.size();

    beginInsertRows(QModelIndex(), newRow, newRow);
    m_records.append(record);
    endInsertRows();
}

const ConsoleMessageRecord& ConsoleMessageModel::recordAt(int row) const
{
    return m_records.at(row);
}

bool ConsoleMessageModel::lessThan(const ConsoleMessageRecord& a,
                                   const ConsoleMessageRecord& b,
                                   int column,
                                   Qt::SortOrder order)
{
    auto cmp = [&](const QString& lhs, const QString& rhs) {
        return order == Qt::AscendingOrder ? lhs < rhs : lhs > rhs;
    };

    switch (column)
    {
        case 0: return cmp(a.timestamp, b.timestamp);
        case 1: return cmp(a.direction, b.direction);
        case 2: return cmp(a.deviceName, b.deviceName);
        case 3: return cmp(a.messageName, b.messageName);
        case 4: return cmp(a.messageId, b.messageId);
        case 5: return cmp(a.dlc, b.dlc);
        case 6: return cmp(a.data, b.data);
        default: return false;
    }
}

void ConsoleMessageModel::sort(int column, Qt::SortOrder order)
{
    beginResetModel();

    std::sort(m_records.begin(), m_records.end(),
              [column, order](const ConsoleMessageRecord& a, const ConsoleMessageRecord& b)
              {
                  return lessThan(a, b, column, order);
              });

    endResetModel();
}