#ifndef MAIN_VIEW_H
#define MAIN_VIEW_H

#include <QWidget>
#include <deque>

namespace Ui {
class MainView;
}
class GraphWindow;
class DataPoint;

class MainView : public QWidget
{
    Q_OBJECT

public:
    explicit MainView(QWidget *parent = nullptr);
    ~MainView();

    Ui::MainView* getUi() const;
    void initializeUiValues();

public slots:
    void updateGraph(const std::deque<DataPoint>& data);
    void setRunningState(bool isRunning);
    void setAutoScroll(bool enabled);

signals:
    // ui control의 이벤트를 외부로 전달하는 시그널들
    void startStopClicked();
    void settingsClicked();

    // 파라미터 변경 시그널
    void amplitudeChanged(double value);
    void currentAmplitudeChanged(double value);
    void frequencyChanged(double value);
    void currentPhaseChanged(int degrees);
    void timeScaleChanged(double value);
    void samplingCyclesChanged(double value);
    void samplesPerCycleChanged(int value);
    void updateModeChanged();

    // 그래프 관련 시그널
    void autoScrollToggled(bool enabled);
    void pointHovered(const QPointF& point);
    void redrawNeeded();

private:
    Ui::MainView *ui;
};

#endif // MAIN_VIEW_H
