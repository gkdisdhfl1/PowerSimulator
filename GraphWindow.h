#ifndef GRAPHWINDOW_H
#define GRAPHWINDOW_H

#include <QDialog>

namespace Ui {
class graphwindow;
}

class graphwindow : public QDialog
{
    Q_OBJECT

public:
    explicit graphwindow(QWidget *parent = nullptr);
    ~graphwindow();

private:
    Ui::graphwindow *ui;
};

#endif // GRAPHWINDOW_H
