#ifndef COMS_H
#define COMS_H

#include <QDialog>

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

signals:
    void typeChanged(int index);

private:
    Ui::Coms *ui;
};

#endif
