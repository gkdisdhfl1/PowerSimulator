#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "GraphWindow.h"
#include "SettingsDialog.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_settingButton_clicked();
    void onDialValueChanged(int value);
    void onSpinBoxValueChanged(double value);

private:
    GraphWindow *m_graphWindow; // 멤버 변수로 선언
    Ui::MainWindow *ui;

    double m_currentVoltageValue = 220.0;
};
#endif // MAINWINDOW_H
