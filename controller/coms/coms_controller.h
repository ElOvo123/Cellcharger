#ifndef COMS_CONTROLLER_H
#define COMS_CONTROLLER_H

#include <QObject>
#include "coms.h"
#include "coms_backend.h"

class ComsController : public QObject
{
    Q_OBJECT

public:
    explicit ComsController(Coms *view, QObject *parent = nullptr);

private slots:
    void onTypeChanged(int index);

private:
    Coms *m_view;
    ComsBackend backend;
};

#endif
