#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QTimer>

// 전방 선언
class GraphWindow;
class SettingsDialog;
class SimulationEngine;
class ValueControlWidget;

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
    void updateAutoRotation();

private:
    Ui::MainWindow *ui;
    GraphWindow *m_graphWindow;
    SettingsDialog *m_settingsDialog;
    SimulationEngine *m_engine;
    QTimer m_autoRotateTimer;

    double m_rotationSpeedHz;
    double m_currentPhaseDegrees; // 현재 위상
};
#endif // MAIN_WINDOW_H
