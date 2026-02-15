#ifndef COMS_H
#define COMS_H

#include <QDialog>

namespace Ui {
class Coms;
}

class Coms : public QDialog
{
    Q_OBJECT

public:
    explicit Coms(QWidget *parent = nullptr);
    ~Coms();

private:
    Ui::Coms *ui;
};

#endif
