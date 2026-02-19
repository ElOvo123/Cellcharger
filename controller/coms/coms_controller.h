#ifndef COMS_CONTROLLER_H
#define COMS_CONTROLLER_H

#include <QObject>

#include "coms.h"
#include "coms_backend.h"

class ComsController : public QObject
{
    Q_OBJECT

public:
    explicit ComsController(Coms *view, ComsBackend *backend, QObject *parent = nullptr);

private slots:
    void onTypeChanged(int index);
    void onConnectToggled(bool connected);
    void onBackendStateChanged(ComsBackend::State state);

private:
    Coms *m_view;
    ComsBackend *m_backend;
};

#endif
