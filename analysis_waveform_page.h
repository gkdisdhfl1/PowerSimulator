#ifndef ANALYSIS_WAVEFORM_PAGE_H
#define ANALYSIS_WAVEFORM_PAGE_H

#include "base_graph_window.h"
#include "measured_data.h"

#include <QWidget>

class AnalysisWaveformPage : public BaseGraphWindow
{
    Q_OBJECT
public:
    explicit AnalysisWaveformPage(QWidget *parent = nullptr);
    void setupSeries() override;

public slots:
    void updateWaveformData(const OneSecondSummaryData& data);

private:
    QLineSeries* m_voltageSeries;
    QLineSeries* m_currentSeries;
    QValueAxis* m_axisY;
};

#endif // ANALYSIS_WAVEFORM_PAGE_H
