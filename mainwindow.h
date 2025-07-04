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
    void onDialMoved(int newDialValue);

private:
    GraphWindow *m_graphWindow; // 멤버 변수로 선언
    Ui::MainWindow *ui;

    double m_currentVoltageValue;
    int m_lastDialValue; // 이전 다이얼 위치를 저장할 변수
};
#endif // MAINWINDOW_H
