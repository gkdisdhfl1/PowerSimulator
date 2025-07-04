#include "GraphWindow.h"
#include "ui_GraphWindow.h"

GraphWindow::GraphWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::GraphWindow)
{
    ui->setupUi(this);
}

GraphWindow::~GraphWindow()
{
    delete ui;
}
