#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "GraphWindow.h"
#include "SettingsDialog.h"

#include "datapoint.h"
#include <deque>
#include <QList>

class QTimer;
class QElapsedTimer;
class QPointF;


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

signals:
    void dataUpdated(const QList<QPointF>& points); // 그래프에 전달할 시그널

private slots:
    void on_settingButton_clicked();
    void onDialMoved(int newDialValue);
    void on_startStopButton_clicked(); // 시작/중지 버튼 슬롯
    void captureData(); // 데이터 캡처를 위한 슬롯
    void applySettings(double interval, int maxSize); // 설정 적용 슬롯

private:
    GraphWindow *m_graphWindow; // 멤버 변수로 선언
    Ui::MainWindow *ui;

    QTimer *m_captureTimer; // 데이터 캡처 타이머
    QElapsedTimer *m_elapsedTimer; // 경과 시간 측정 타이머

    std::deque<DataPoint> m_data; // 데이터 저장

    int m_maxDataSize; // 데이터 최대 저장 개수
    double m_currentVoltageValue;
    int m_lastDialValue; // 이전 다이얼 위치를 저장할 변수


};
#endif // MAINWINDOW_H
