#pragma once

#include <QObject>
#include "icomms_backend.h"
#include "coms.h"

class ComsController : public QObject
{
    Q_OBJECT

public:
    explicit ComsController(Coms *view, IComsBackend *backend, QObject *parent = nullptr);

private slots:
    void onTypeChanged(int);
    void onConnectToggled(bool connected);
    void onBackendStateChanged(IComsBackend::State state);
    void onBackendStatusMessage(const QString &message);
    void onBackendMessageReceived(const QString &message);

private:
    Coms *m_view;
    IComsBackend *m_backend;
};