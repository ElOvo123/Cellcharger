#include "console_message_filter_proxy_model.h"

#include <QAbstractItemModel>

ConsoleMessageFilterProxyModel::ConsoleMessageFilterProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}

void ConsoleMessageFilterProxyModel::setTextFilters(const QSet<QString>& filters)
{
    m_textFilters = filters;
    invalidateFilter();
}

void ConsoleMessageFilterProxyModel::setDeviceFilters(const QSet<QString>& filters)
{
    m_deviceFilters = filters;
    invalidateFilter();
}

void ConsoleMessageFilterProxyModel::setMessageIdFilters(const QSet<QString>& filters)
{
    m_messageIdFilters = filters;
    invalidateFilter();
}

void ConsoleMessageFilterProxyModel::setMessageNameFilters(const QSet<QString>& filters)
{
    m_messageNameFilters = filters;
    invalidateFilter();
}

bool ConsoleMessageFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    const QModelIndex timeIdx = sourceModel()->index(source_row, 0, source_parent);
    const QModelIndex dirIdx = sourceModel()->index(source_row, 1, source_parent);
    const QModelIndex devIdx = sourceModel()->index(source_row, 2, source_parent);
    const QModelIndex nameIdx = sourceModel()->index(source_row, 3, source_parent);
    const QModelIndex msgIdIdx = sourceModel()->index(source_row, 4, source_parent);
    const QModelIndex dlcIdx = sourceModel()->index(source_row, 5, source_parent);
    const QModelIndex dataIdx = sourceModel()->index(source_row, 6, source_parent);

    const QString time = sourceModel()->data(timeIdx).toString();
    const QString direction = sourceModel()->data(dirIdx).toString();
    const QString device = sourceModel()->data(devIdx).toString();
    const QString msgName = sourceModel()->data(nameIdx).toString();
    const QString msgId = sourceModel()->data(msgIdIdx).toString();
    const QString dlc = sourceModel()->data(dlcIdx).toString();
    const QString data = sourceModel()->data(dataIdx).toString();

    if (!m_deviceFilters.isEmpty() && !m_deviceFilters.contains(device))
        return false;

    if (!m_messageIdFilters.isEmpty() && !m_messageIdFilters.contains(msgId))
        return false;

    if (!m_messageNameFilters.isEmpty() && !m_messageNameFilters.contains(msgName))
        return false;

    if (!m_textFilters.isEmpty())
    {
        const QString searchable =
            time + " " + direction + " " + device + " " + msgName + " " + msgId + " " + dlc + " " + data;

        bool matched = false;
        for (const QString& filter : m_textFilters)
        {
            if (searchable.contains(filter, Qt::CaseInsensitive))
            {
                matched = true;
                break;
            }
        }

        if (!matched)
            return false;
    }

    return true;
}