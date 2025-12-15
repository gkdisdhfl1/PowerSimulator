#ifndef DATA_PAGE_H
#define DATA_PAGE_H

#include "demand_data.h"
#include "measured_data.h"
#include "datasource_types.h"
#include <QWidget>

class OneSecondSummaryData;
class DataRowWidget;
class QButtonGroup;
class QPushButton;
class QLabel;

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
    void onDemandDataUpdated(const DemandData& data);

private slots:
    void onModeChanged(int id);
    void onMinMaxModeChanged(int id);

private:
    void updateDisplay();
    // 버튼 상태 관리 헬퍼 함수 (static 함수로 만들면 this 포인터가 필요 없어 깔끔)
    static bool updateButtonState(QPushButton* button, bool hasData);

    std::vector<DataRowWidget*> m_rowWidgets;
    std::vector<DataSource> m_dataSources;
    int m_currentSourceIndex = 0;

    QString m_originalTitle;
    QLabel* m_titleLabel = nullptr;

    QButtonGroup* m_modeButtonGroup = nullptr;
    QButtonGroup* m_minMaxButtonGroup = nullptr;

    OneSecondSummaryData m_lastData;
    DemandData m_lastDemandData;
};

#endif // DATA_PAGE_H
