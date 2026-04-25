#ifndef COMS_H
#define COMS_H

#include <QColor>
#include <QDialog>
#include <QList>

#include "coms_types.h"

namespace Ui
{
class Coms;
}

struct ComsConnectionInfo
{
    ComsType type = ComsType::Serial;
    ComsConfig config;
    QColor indicatorColor;
    bool connected = false;
};

class Coms : public QDialog
{
    Q_OBJECT

public:
    explicit Coms(QWidget* parent = nullptr);
    ~Coms();

    ComsType connectionType(int row) const;
    ComsConfig connectionConfig(int row) const;
    void setConnections(const QList<ComsConnectionInfo>& connections);
    void setStatusText(const QString& text);
    void setOverallConnected(bool connected);
    void pulseReceiveActivity();

signals:
    void addConnectionRequested();
    void connectConnectionRequested(int row);
    void disconnectConnectionRequested(int row);
    void removeConnectionRequested(int row);

private:
    struct ConnectionRowWidgets;

    void ensureConnectionRows(int count);
    void rebuildRowIndices();
    void updateRowWidget(int row, const ComsConnectionInfo& connection);

private:
    Ui::Coms* ui;
    QList<ConnectionRowWidgets*> m_rowWidgets;
};

#endif
