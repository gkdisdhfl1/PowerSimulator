#include "analysis_harmonic_page.h"
#include "analysis_utils.h"

#include <QBarCategoryAxis>
#include <QBarSeries>
#include <QBarSet>
#include <QButtonGroup>
#include <QCategoryAxis>
#include <QChart>
#include <QChartView>
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QValueAxis>
#include <config.h>

AnalysisHarmonicPage::AnalysisHarmonicPage(QWidget *parent)
    : QWidget{parent}
    , m_voltageButton(nullptr)
    , m_currentButton(nullptr)
    , m_buttonGroup(nullptr)
{
    setupUi();
}
// public

void AnalysisHarmonicPage::onOneSecondDataUpdated(const OneSecondSummaryData& data)
{
    m_lastSummaryData = data;
    m_hasData = true;
    updateGraph();
}

// private
void AnalysisHarmonicPage::setupUi()
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);
    mainLayout->setSpacing(2);

    // 1. 상단 바 설정 (제목, Voltage/Current 버튼)
    setupTopBar(mainLayout);

    // 2. 구분선
    QFrame* titleLine = new QFrame(this);
    titleLine->setFrameShape(QFrame::HLine);
    titleLine->setObjectName("titleLine");
    titleLine->setFixedHeight(2);
    mainLayout->addWidget(titleLine);

    // 3. 컨트롤 바 설정
    setupControlBar(mainLayout);

    // 4. 컨텐츠 스택 위젯
    m_contentStack = new QStackedWidget();
    mainLayout->addWidget(m_contentStack, 1); // 남은 공간을 모두 차지

    // Graph 뷰와 Text 뷰 생성 및 추가
    QWidget* graphView = createGraphView();
    QWidget* textView = createTextView(); // 지금은 빈 위젯으로 생성
    m_contentStack->addWidget(graphView);
    m_contentStack->addWidget(textView);

    connect(m_viewTypeComboBox, &QComboBox::currentIndexChanged, m_contentStack, &QStackedWidget::setCurrentIndex);
    connect(m_dataTypeComboBox, &QComboBox::currentIndexChanged, this, &AnalysisHarmonicPage::updateChartAxis);
}

void AnalysisHarmonicPage::updateGraph()
{
    if(!m_hasData) return;

    const auto* fullHarmonicsData = m_voltageButton->isChecked() ?
                                        &m_lastSummaryData.lastCycleFullVoltageHarmonics :
                                        &m_lastSummaryData.lastCycleFullCurrentHarmonics;

    const auto* thdData = m_voltageButton->isChecked() ?
                              &m_lastSummaryData.voltageThd :
                              &m_lastSummaryData.currentThd;

    const auto* fundData = m_voltageButton->isChecked() ?
                               &m_lastSummaryData.fundamentalVoltage :
                               &m_lastSummaryData.fundamentalCurrent;

    // 데이터 타입 콤보박스 선택에 따라 값 계산 및 그래프 업데이트
    int dataTypeIndex = m_dataTypeComboBox->currentIndex();
    double maxVal = 0.0; // 자동 스케일링을 위한 최대값

    for(int i{0}; i < 3; ++i) { // A상, B상, C상
        const auto& harmonics = (*fullHarmonicsData)[i];

        if(i == 0) m_thdValueLabels[i]->setText(QString::number(thdData->a, 'f', 1));
        else if(i == 1) m_thdValueLabels[i]->setText(QString::number(thdData->b, 'f', 1));
        else m_thdValueLabels[i]->setText(QString::number(thdData->c, 'f', 1));

        m_fundValueLabels[i]->setText(QString::number((*fundData)[i].rms, 'f', 1));

        // 막대 그래프 데이터 업데이트
        // 0차부터 50차까지 데이터 준비
        for(int order{0}; order <= 50; ++order) {
            double rawValue = 0.0;
            double displayValue = 0.0;
            auto it = std::find_if(harmonics.begin(), harmonics.end(), [order](const HarmonicAnalysisResult& h) {
                return h.order == order;
            });

            if(it != harmonics.end()) {
                const auto& harmonic = *it;
                if(dataTypeIndex == 0) { // voltage or current
                    rawValue = harmonic.rms;
                } else if(dataTypeIndex == 1) { // %RMS
                    rawValue = 0.0; // 임시
                } else { // Fund
                    const auto& fundamental = (*fundData)[i];
                    if(fundamental.rms > 1e-9) {
                        rawValue = (harmonic.rms / fundamental.rms) * 100.0;
                    }
                }
            }

            displayValue = (dataTypeIndex == 0) ? AnalysisUtils::scaleValue(rawValue, m_scaleUnit) : rawValue;

            if(order < m_barSets[i]->count()) {
                m_barSets[i]->replace(order, displayValue);
            }
            if(order > 0) { // DC 성분은 자동 스케일링에서 제외
                maxVal = std::max(maxVal, rawValue);
            }
        }
    }

    // 자동 스케일링
    if(m_isAutoScaling) {
        int newScaleIndex = m_scaleIndex;
        for(size_t i{0}; i < config::View::RANGE_TABLE.size(); ++i) {
            if(maxVal <= config::View::RANGE_TABLE[i]) {
                newScaleIndex = i;
                break;
            }
        }
        if(newScaleIndex != m_scaleIndex) {
            m_scaleIndex = newScaleIndex;
            updateChartAxis();
        }
    }
}

void AnalysisHarmonicPage::setupTopBar(QVBoxLayout* mainLayout)
{
    auto topLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel("Harmonics");
    titleLabel->setObjectName("titleLabel");
    topLayout->addWidget(titleLabel);
    topLayout->addStretch(); // 제목과 버튼 사이 공간

    // Voltage/Current 토글 버튼
    m_voltageButton = new QPushButton("Voltage");
    m_currentButton = new QPushButton("Current");
    m_voltageButton->setCheckable(true);
    m_currentButton->setCheckable(true);

    // 버튼에 스타일 속성 부여
    m_voltageButton->setObjectName("harmonicsToggle");
    m_currentButton->setObjectName("harmonicsToggle");

    // QButtonGroup으로 상호 배타적 동작 설정
    m_buttonGroup = new QButtonGroup(this);
    m_buttonGroup->setExclusive(true); // 하나만 선택 가능
    m_buttonGroup->addButton(m_voltageButton, 0); // ID 0
    m_buttonGroup->addButton(m_currentButton, 1); // ID 1

    // 초기 선택
    m_voltageButton->setChecked(true); // Voltage가 기본 선택

    // 버튼을 topLayout에 추가
    topLayout->addWidget(m_voltageButton);
    topLayout->addSpacing(5);
    topLayout->addWidget(m_currentButton);

    connect(m_buttonGroup, &QButtonGroup::idClicked, this, &AnalysisHarmonicPage::onDisplayTypeChanged);

    mainLayout->addLayout(topLayout);
}

void AnalysisHarmonicPage::setupControlBar(QVBoxLayout* mainLayout)
{
    auto controlBarLayout = new QHBoxLayout();
    mainLayout->addLayout(controlBarLayout);

    // 데이터 종류 콤보박스
    m_dataTypeComboBox = new QComboBox();
    m_dataTypeComboBox->addItems({"Voltage", "[%]RMS", "[%]Fund"});
    m_dataTypeComboBox->setObjectName("harmonicsCombobox");
    controlBarLayout->addWidget(m_dataTypeComboBox);

    // 표시 형식 콤보박스
    m_viewTypeComboBox = new QComboBox();
    m_viewTypeComboBox->addItems({"Graph", "Text"});
    m_viewTypeComboBox->setObjectName("harmonicsCombobox");
    controlBarLayout->addWidget(m_viewTypeComboBox);

    controlBarLayout->addSpacing(30);

    // Fund. 체크박스
    m_fundCheckBox = new QCheckBox("Fund.");
    m_fundCheckBox->setChecked(true);
    m_fundCheckBox->setProperty("checkType", "fundCheck");

    controlBarLayout->addWidget(m_fundCheckBox);
    controlBarLayout->addStretch();

    // A, B, C 상 체크박스
    const QStringList phaseNames = {"A", "B", "C"};
    for(int i{0}; i < 3; ++i) {
        m_phaseCheckBoxes[i] = new QCheckBox(phaseNames[i]);
        m_phaseCheckBoxes[i]->setChecked(true);
        m_phaseCheckBoxes[i]->setProperty("checkType", "phaseCheck");
        QString phaseStyle = QString("QCheckBox::checked { background-color: %1; }")
                                 .arg(config::View::PhaseColors::Voltage[i].name());
        m_phaseCheckBoxes[i]->setStyleSheet(phaseStyle);

        controlBarLayout->addWidget(m_phaseCheckBoxes[i]);
    }


}

QWidget* AnalysisHarmonicPage::createGraphView()
{
    auto graphViewWidget = new QWidget();
    auto mainLayout = new QHBoxLayout(graphViewWidget);
    mainLayout->setContentsMargins(0, 10, 0, 0); // 컨트롤 바와의 간격

    // 1. 왼쪽 스케일 패널
    auto scaleBox = new QGroupBox("Scale");
    scaleBox->setObjectName("scaleBox");
    scaleBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    auto scaleLayout = new QVBoxLayout(scaleBox);
    scaleLayout->setContentsMargins(2, 2, 2, 2);

    m_autoScaleButton = new QPushButton("Auto");
    auto plusButton = new QPushButton("+");
    auto minusButton = new QPushButton("-");

    m_autoScaleButton->setCheckable(true);
    m_autoScaleButton->setChecked(m_isAutoScaling);
    scaleLayout->addWidget(m_autoScaleButton);
    scaleLayout->addWidget(plusButton);
    scaleLayout->addWidget(minusButton);
    scaleLayout->addStretch();

    connect(m_autoScaleButton, &QPushButton::toggled, this, &AnalysisHarmonicPage::onScaleAutoToggled);
    connect(plusButton, &QPushButton::clicked, this, &AnalysisHarmonicPage::onScaleInClicked);
    connect(minusButton, &QPushButton::clicked, this, &AnalysisHarmonicPage::onScaleOutClicked);

    mainLayout->addWidget(scaleBox);

    // 2. 오른쪽 그래프 및 정보 패널
    auto rightPanel = new QWidget();
    auto rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(10, 0, 0, 0);

    // 2-1. 상단 정보 영역 (THD, Fund.)
    auto infoContainer = new QWidget();
    auto infoLayout = new QGridLayout(infoContainer);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    // infoLayout->setSpacing(5);

    const QStringList phaseNames = {"A", "B", "C"};

    // THD 라인
    QLabel* thdLabel = new QLabel("THD");
    thdLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLayout->addWidget(thdLabel, 0, 0);
    for(int i{0}; i < 3; ++i) {
        int col = 1 + (i * 3); // A, B, C 각 그룹의 시작 컬럼
        infoLayout->addWidget(new QLabel(phaseNames[i]), 0, col);
        m_thdValueLabels[i] = new QLabel("0.00");
        infoLayout->addWidget(m_thdValueLabels[i], 0, col + 1);
        infoLayout->addWidget(new QLabel("%"), 0, col + 2);
    }

    // Fund. 라인
    QLabel* fundLabel = new QLabel("Fund.");
    fundLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLayout->addWidget(fundLabel, 1, 0);
    for(int i{0}; i < 3; ++i) {
        int col = 1 + (i * 3); // A, B, C 각 그룹의 시작 컬럼
        infoLayout->addWidget(new QLabel(phaseNames[i]), 1, col);
        m_fundValueLabels[i] = new QLabel("0.00");
        infoLayout->addWidget(m_fundValueLabels[i], 1, col + 1);
        infoLayout->addWidget(new QLabel("V"), 1, col + 2);
    }

    infoLayout->setColumnStretch(0, 1);

    rightLayout->addWidget(infoContainer);

    // 2-2. 하단 막대 그래프
    m_chart = new QChart();
    m_chart->legend()->hide();
    m_chartView = new QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);

    // Y축 단위 라벨 추가
    auto* internalVLayout = new QVBoxLayout(m_chartView);
    auto* chartHeaderLayout = new QHBoxLayout();
    internalVLayout->addLayout(chartHeaderLayout);
    internalVLayout->addStretch();

    m_unitLabel = new QLabel("[V]"); // 초기 단위는 Voltage
    QFont axisFont;
    axisFont.setPixelSize(8);
    m_unitLabel->setFont(axisFont);
    m_unitLabel->setObjectName("axisUnitLabel");

    // chartHeaderLayout->addStretch();
    chartHeaderLayout->addWidget(m_unitLabel);
    chartHeaderLayout->setContentsMargins(15, 0, 15, 0);

    // Y축 생성
    m_axisY = new QValueAxis();
    m_axisY->setLabelsFont(axisFont);
    m_axisY->setRange(0, config::View::RANGE_TABLE[m_scaleIndex]);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);

    // X축 생성
    m_axisX = new QCategoryAxis();
    m_axisX->setRange(0, 50); // 0부터 50까지 모두 보이도록 설정
    m_axisX->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);

    for(int i{1}; i <= 5; ++i)
        m_axisX->append(QString("%1").arg(8 * i), 8 * i);
    m_axisX->append("50", 50);

    m_axisX->setGridLineVisible(false); // 세로 그리드 라인 숨기기
    m_axisX->setTickCount(7);

    m_chart->addAxis(m_axisX, Qt::AlignBottom);

    // 막대 시리즈 및 세트 생성
    m_barSeries = new QBarSeries();
    for(int i{0}; i < 3; ++i) {
        m_barSets[i] = new QBarSet(QString(QChar('A' + i)));
        // 초기 데이터는 모두 0으로 채움
        for(int j{0}; j <= 50; ++j) {
            *m_barSets[i] << 0.0;
        }
        m_barSets[i]->setColor(config::View::PhaseColors::Voltage[i]);
        m_barSeries->append(m_barSets[i]);
    }
    m_chart->addSeries(m_barSeries);
    m_barSeries->attachAxis(m_axisX);
    m_barSeries->attachAxis(m_axisY);

    rightLayout->addWidget(m_chartView, 1);

    mainLayout->addWidget(rightPanel, 1); // 오른쪽 패널이 남은 공간 모두 차지

    updateChartAxis(); // 생성 시 한번 호출
    return graphViewWidget;
}

QWidget* AnalysisHarmonicPage::createTextView()
{
    // 지금은 빈 위젯을 반환. 나중에 구현
    auto textViewWidget = new QWidget();
    auto layout = new QVBoxLayout(textViewWidget);
    layout->addWidget(new QLabel("Text View (구현 예정)"));
    return textViewWidget;
}

//-------------------------

void AnalysisHarmonicPage::onDisplayTypeChanged(int id)
{
    if(id == 0) { // Voltage 버튼 클릭
        m_dataTypeComboBox->setItemText(0, "Voltage");
    } else { // Current 버튼 클릭
        m_dataTypeComboBox->setItemText(0, "Current");
    }
    updateChartAxis();
    updateGraph();
}

void AnalysisHarmonicPage::onScaleAutoToggled(bool checked)
{
    m_isAutoScaling = checked;
    updateGraph();
}

void AnalysisHarmonicPage::onScaleInClicked()
{
    if(m_scaleIndex < (int)config::View::RANGE_TABLE.size() - 1) {
        ++m_scaleIndex;
        m_autoScaleButton->setChecked(false);
        updateChartAxis();
        updateGraph();
    }
}

void AnalysisHarmonicPage::onScaleOutClicked()
{
    if(m_scaleIndex > 0) {
        --m_scaleIndex;
        m_autoScaleButton->setChecked(false);
        updateChartAxis();
        updateGraph();
    }
}

void AnalysisHarmonicPage::updateChartAxis()
{
    if(!m_axisY || !m_unitLabel) return;

    // Y축 단위 결정
    QString unitText;
    bool isVoltage = m_voltageButton->isChecked();
    qDebug() << "isVoltage: " << isVoltage;

    int dataTypeIndex = m_dataTypeComboBox->currentIndex();
    qDebug() << "dataTypeIndex: " << dataTypeIndex;


    // 2. 현재 범위와 단위를 계산
    double currentRange = config::View::RANGE_TABLE[m_scaleIndex];
    m_scaleUnit = AnalysisUtils::updateScaleUnit(currentRange);

    qDebug() << "m_scaleIndex; " << m_scaleIndex;
    qDebug() << "currentRange: " << currentRange;


    // 3. 단위 텍스트 설정
    if(dataTypeIndex == 0) { // Voltage or Current
        QString baseUnit = isVoltage ? "V" : "A";
        // qDebug() << "baseUnit: " << baseUnit;
        if(m_scaleUnit == ScaleUnit::Milli) {
            // qDebug() << "m_scaleUnit: Milli";
            unitText = QString("m%1").arg(baseUnit);
        }
        else if(m_scaleUnit == ScaleUnit::Kilo) {
            // qDebug() << "m_scaleUnit: Kilo";
            unitText = QString("k%1").arg(baseUnit);
        }
        else {
            // qDebug() << "m_scaleUnit: normal";
            unitText = baseUnit;
        }
    } else if(dataTypeIndex == 1){
        unitText = "%RMS";
    } else {
        unitText = "%Fund";
    }

    // qDebug() << "unitText: " << unitText;
    m_unitLabel->setText(QString("[%1]").arg(unitText));
    // qDebug() << "m_unitLabel: " << m_unitLabel->text();

    // 4. Y축 범위 설정
    double displayRange = AnalysisUtils::scaleValue(currentRange, m_scaleUnit);
    // qDebug() << "displayRange: " << displayRange;
    m_axisY->setRange(0, displayRange);
    updateGraph();
    qDebug() << "-----------------------------------";

}
