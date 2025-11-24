#include "data_page.h"
#include "data_row_widget.h"

#include <QButtonGroup>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

DataPage::DataPage(const QString& title,
                   const QString& unit,
                   const std::vector<DataSource>& dataSources,
                   QWidget* parent)
    : QWidget{parent}
    , m_dataSources(dataSources)
{   
    auto mainlayout = new QVBoxLayout(this);
    mainlayout->setContentsMargins(15, 10, 15, 10);
    mainlayout->setSpacing(0);

    // 상단 (제목 + 버튼)
    auto topLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel(title, this);
    titleLabel->setObjectName("titleLabel");
    topLayout->addWidget(titleLabel);
    topLayout->addStretch();

    // 1. 모드 전환 버튼 (L-L, L-N)
    if(m_dataSources.size() > 1) {
        m_modeButtonGroup = new QButtonGroup(this);
        m_modeButtonGroup->setExclusive(true);

        for(int i{0}; i < m_dataSources.size(); ++i) {
            topLayout->addSpacing(3);
            QPushButton* button = new QPushButton(m_dataSources[i].name);
            button->setCheckable(true);
            button->setObjectName("modeButton");

            topLayout->addWidget(button);
            m_modeButtonGroup->addButton(button, i);
        }

        m_modeButtonGroup->button(0)->setChecked(true); // 첫 번째 버튼을 기본값으로 선택
        connect(m_modeButtonGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, &DataPage::onModeChanged);
    }

    // 2. Min/Max 버튼 생성
    if(!m_dataSources.empty()) {

        m_minMaxButtonGroup = new QButtonGroup(this);
        m_minMaxButtonGroup->setExclusive(false); // 하나만 선택되거나 모두 선택 해제될 수 있음

        auto maxButton = new QPushButton("Max");
        maxButton->setCheckable(true);
        m_minMaxButtonGroup->addButton(maxButton, 0);

        auto minButton = new QPushButton("Min");
        minButton->setCheckable(true);
        m_minMaxButtonGroup->addButton(minButton, 1);

        connect(m_minMaxButtonGroup, &QButtonGroup::idClicked, this, &DataPage::onMinMaxModeChanged);

        topLayout->addWidget(maxButton);
        topLayout->addWidget(minButton);
    }

    mainlayout->addLayout(topLayout);
    mainlayout->addSpacing(5);

    // 제목 밑 진한 라인
    QFrame* titleLine = new QFrame(this);
    titleLine->setFrameShape(QFrame::HLine);
    titleLine->setObjectName("titleLine");
    titleLine->setFixedHeight(2);
    mainlayout->addWidget(titleLine);

    // 모든 데이터 소스의 rowLabels 중 가장 긴 것의 크기를 찾음
    size_t maxRows = 0;
    if(!m_dataSources.empty()) {
        auto it = std::max_element(m_dataSources.begin(), m_dataSources.end(),
                                   [](const DataSource& a, const DataSource& b) {
            return a.rowLabels.size() < b.rowLabels.size();
        });
        if(it != m_dataSources.end()) {
            maxRows = it->rowLabels.size();
        }
    }
    m_rowWidgets.reserve(maxRows);
    for(size_t i = 0; i < maxRows; ++i) {
        bool hasLine = (i != maxRows - 1); // 마지막 행은 line 없음

        QString initialLabel = (i < m_dataSources[0].rowLabels.size()) ? m_dataSources[0].rowLabels[i] : "";
        DataRowWidget* row = new DataRowWidget(initialLabel, unit, hasLine, this);
        mainlayout->addWidget(row);
        m_rowWidgets.push_back(row); // 행 위젯 포인터 저장
    }

    mainlayout->addStretch(); // 모든 요소를 밀착

    updateDisplay();
}

void DataPage::onDataUpdated(const OneSecondSummaryData& data)
{
    m_lastData = data;
    updateDisplay();
}

void DataPage::onModeChanged(int id)
{
    if(id < m_dataSources.size()) {
        m_currentSourceIndex = id;
        updateDisplay();
    }
}

void DataPage::onDemandDataUpdated(const DemandData& data)
{
    m_lastDemandData = data;
    updateDisplay();
}

void DataPage::onMinMaxModeChanged(int id)
{
    auto* clickedButton = m_minMaxButtonGroup->button(id);
    if(clickedButton && clickedButton->isChecked()) {
        // 버튼이 선택되면 그룹 내 다른 버튼들은 선택 해제
        for(auto* button : m_minMaxButtonGroup->buttons()) {
            if(button != clickedButton) {
                bool wasBlocked = button->blockSignals(true);
                button->setChecked(false);
                button->blockSignals(wasBlocked);
            }
        }
    }
    updateDisplay();
}

void DataPage::updateDisplay()
{
    if(m_dataSources.empty()) return;

    const auto& currentSource = m_dataSources[m_currentSourceIndex];
    const auto& currentLabels = currentSource.rowLabels;
    const auto& currentExtractors = currentSource.extractors;

    bool showMax = false;
    bool showMin = false;

    if(m_minMaxButtonGroup) {
        QPushButton* maxButton = qobject_cast<QPushButton*>(m_minMaxButtonGroup->button(0));
        QPushButton* minButton = qobject_cast<QPushButton*>(m_minMaxButtonGroup->button(1));

        // 버튼 가시성/상태 제어 및 현재 활성화 여부 반환 람다
        auto manageButtonState = [](QPushButton* button, bool hasData) -> bool {
            if(!button) return false;

            button->setVisible(hasData);
            if(!hasData && button->isChecked()) {
                button->setChecked(false);
            }

            // 버튼이 보이고(데이터가 있고) 체크가 되어있어야 활성화 된 것
            return hasData && button->isChecked();
        };

        // 데이터 존재 여부에 따라 버튼 상태를 갱신하고, 표시 여부를 결정
        showMax = manageButtonState(maxButton, !currentSource.maxExtractors.empty());
        showMin = manageButtonState(minButton, !currentSource.minExtractors.empty());

    }

    // 모든 행 위젯을 순회
    for(int i{0}; i < m_rowWidgets.size(); ++i) {
        if(i < currentLabels.size()) {
            // 현재 소스에 해당하는 행이면, 보이게 하고 라벨 텍스트 업데이트
            m_rowWidgets[i]->setVisible(true);
            m_rowWidgets[i]->setLabel(currentLabels[i]);

            if(showMax && i < currentSource.maxExtractors.size()) {
                auto valWithTime = currentSource.maxExtractors[i](m_lastDemandData);
                m_rowWidgets[i]->setValue(valWithTime.value, valWithTime.timestamp);
            } else if(showMin && i < currentSource.minExtractors.size()) {
                auto valWithTime = currentSource.minExtractors[i](m_lastDemandData);
                m_rowWidgets[i]->setValue(valWithTime.value, valWithTime.timestamp);
            } else if(i < currentExtractors.size()) {
                double value = currentExtractors[i](m_lastData);
                m_rowWidgets[i]->setValue(value);
            }
        } else {
            // 해당하지 않는 행이면 숨김
            m_rowWidgets[i]->setVisible(false);
        }
    }
}
