#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_graphWindow = new graphwindow(this);
    m_graphWindow->show();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_settingButton_clicked()
{
    settingsdialog dialog(this);
    // dialog.setInitialValues(...); 나중에 추가할 부분

    // 이 부분도 나중에 추가
    // connect(&dialog, &SettingsDialog::settingsApplied, this, &MainWindow::applySettings);
    dialog.exec();
}

