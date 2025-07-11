#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

// 전방 선언
class GraphWindow;
class SettingsDialog;
class SimulationEngine;
class FineTuningDial;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(SimulationEngine *engine, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_settingButton_clicked();
    void toggleFineTuningMode(); // FineTuningDial의 doubleClicked 시그널과 연결될 슬롯

private:
    Ui::MainWindow *ui;
    GraphWindow *m_graphWindow;
    SettingsDialog *m_settingsDialog;
    SimulationEngine *m_engine;

    bool m_isFineTuningMode = false;
    int m_lastDialValue;
    void updateUiForTuningMode(); // UI 피드백을 위한 함수
};
#endif // MAIN_WINDOW_H
