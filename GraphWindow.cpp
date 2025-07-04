#include "graphwindow.h"
#include "ui_graphwindow.h"

graphwindow::graphwindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::graphwindow)
{
    ui->setupUi(this);
}

graphwindow::~graphwindow()
{
    delete ui;
}
