#pragma once

#include <QSortFilterProxyModel>
#include <QSet>

class ConsoleMessageFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit ConsoleMessageFilterProxyModel(QObject *parent = nullptr);

    void setTextFilters(const QSet<QString>& filters);
    void setDeviceFilters(const QSet<QString>& filters);
    void setMessageIdFilters(const QSet<QString>& filters);
    void setMessageNameFilters(const QSet<QString>& filters);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    QSet<QString> m_textFilters;
    QSet<QString> m_deviceFilters;
    QSet<QString> m_messageIdFilters;
    QSet<QString> m_messageNameFilters;
};