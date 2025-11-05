#ifndef DATA_PAGE_H
#define DATA_PAGE_H

class OneSecondSummaryData;
class DataRowWidget;

#include <QWidget>

using Extractor = std::function<double(const OneSecondSummaryData&)>;

class DataPage : public QWidget
{
    Q_OBJECT
public:
    explicit DataPage(const QString& title,
                      const QStringList& rowLabels,
                      const QString& unit,
                      const std::vector<Extractor>& extractors,
                      QWidget* parent = nullptr);

public slots:
    void updateDataFromSummary(const OneSecondSummaryData& data);

private:
    std::vector<DataRowWidget*> m_rowWidgets;
    std::vector<Extractor> m_extractors;
};

#endif // DATA_PAGE_H
