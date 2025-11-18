#ifndef DATA_PAGE_H
#define DATA_PAGE_H

#include "measured_data.h"
#include <QWidget>

class OneSecondSummaryData;
class DataRowWidget;
class QButtonGroup;


using Extractor = std::function<double(const OneSecondSummaryData&)>;

struct DataSource {
    QString name; // 버튼 이름
    QStringList rowLabels;
    std::vector<Extractor> extractors;
};

class DataPage : public QWidget
{
    Q_OBJECT
public:
    explicit DataPage(const QString& title,
                      const QString& unit,
                      const std::vector<DataSource>& dataSources,
                      QWidget* parent = nullptr);

public slots:
    void onDataUpdated(const OneSecondSummaryData& data);

private slots:
    void onModeChanged(int id);

private:
    void updateDisplay();

    std::vector<DataRowWidget*> m_rowWidgets;
    std::vector<DataSource> m_dataSources;
    int m_currentSourceIndex = 0;

    QButtonGroup* m_modeButtonGroup = nullptr;
    OneSecondSummaryData m_lastData;
};

#endif // DATA_PAGE_H
