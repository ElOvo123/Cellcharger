#ifndef COMS_H
#define COMS_H

#include <QDialog>
#include "coms_config.h"

enum class ComsType
{
    Serial = 0,
    Socket_vcan = 1,
    Socket_UDP = 2,
    Socket_TCP = 3
};

namespace Ui {
class Coms;
}

class Coms : public QDialog
{
    Q_OBJECT

public:
    explicit Coms(QWidget *parent = nullptr);
    ~Coms();

    ComsType currentType() const;
    ComsConfig currentConfig() const;
    void setStatusText(const QString &text);
    void setConnectedUI(bool connected);

signals:
    void typeChanged(int index);
    void connectToggled(bool connected);

private:
    Ui::Coms *ui;
};

#endif
