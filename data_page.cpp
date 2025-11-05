#include "data_page.h"
#include "data_row_widget.h"

#include <QLabel>
#include <QVBoxLayout>

DataPage::DataPage(const QString& title,
                   const QStringList& rowLabels,
                   const QString& unit,
                   const std::vector<Extractor>& extractors,
                   QWidget* parent)
    : QWidget{parent}
    , m_extractors(extractors)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 10, 15, 10);
    layout->setSpacing(0);

    // 제목
    QLabel* titleLabel = new QLabel(title, this);
    titleLabel->setObjectName("titleLabel");

    layout->addWidget(titleLabel);
    layout->addSpacing(5);

    // 제목 밑 진한 라인
    QFrame* titleLine = new QFrame(this);
    titleLine->setFrameShape(QFrame::HLine);
    titleLine->setObjectName("titleLine");
    titleLine->setFixedHeight(2);
    layout->addWidget(titleLine);

    m_rowWidgets.reserve(rowLabels.count());
    for(int i{0}; i < rowLabels.count(); ++i) {
        bool hasLine = (i != rowLabels.count() - 1); // 마지막 행은 line 없음
        DataRowWidget* row = new DataRowWidget(rowLabels[i], unit, hasLine, this);
        layout->addWidget(row);
        m_rowWidgets.push_back(row); // 행 위젯 포인터 저장
    }

    layout->addStretch(); // 모든 요소를 밀착
}

void DataPage::updateDataFromSummary(const OneSecondSummaryData& data)
{
    for(size_t i{0}; i < m_extractors.size(); ++i) {
        if(i < m_rowWidgets.size()) {
            double value = m_extractors[i](data);
            m_rowWidgets[i]->setValue(value); // 각 행 위젯의 슬롯 호출
        }
    }
}
