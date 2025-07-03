#include "settingsdialog.h"
#include "ui_settingsdialog.h"

settingsdialog::settingsdialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::settingsdialog)
{
    ui->setupUi(this);
}

settingsdialog::~settingsdialog()
{
    delete ui;
}
