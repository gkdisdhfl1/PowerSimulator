#ifndef ANALYSIS_WAVEFORM_PAGE_H
#define ANALYSIS_WAVEFORM_PAGE_H

#include "base_graph_window.h"

#include <QWidget>

class SimulationEngine;

class AnalysisWaveformPage : public BaseGraphWindow
{
    Q_OBJECT
public:
    explicit AnalysisWaveformPage(SimulationEngine* engine, QWidget *parent = nullptr);
    void setupSeries() override;

public slots:
    void updateWaveformData(const std::deque<DataPoint>& data);

private:
    QLineSeries* m_voltageSeries;
    QLineSeries* m_currentSeries;
    QValueAxis* m_axisY;
};

#endif // ANALYSIS_WAVEFORM_PAGE_H
