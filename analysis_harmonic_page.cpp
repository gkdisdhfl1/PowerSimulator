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
#include <QTableWidget>
#include <QVBoxLayout>
#include <QValueAxis>
#include <config.h>
#include <QHeaderView>
#include <QTimer>


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
    updateText();
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

    connect(m_viewTypeComboBox, &QComboBox::currentIndexChanged, this, &AnalysisHarmonicPage::onViewTypeChanged);
    connect(m_viewTypeComboBox, &QComboBox::currentIndexChanged, m_contentStack, &QStackedWidget::setCurrentIndex);
    connect(m_dataTypeComboBox, &QComboBox::currentIndexChanged, this, &AnalysisHarmonicPage::updateChartAxis);
}

HarmonicDataSources AnalysisHarmonicPage::getCurrentDataSources(int phaseIndex) const
{
    HarmonicDataSources sources;
    bool isVoltage = m_voltageButton->isChecked();

    const auto& fullHarmonics = isVoltage ?
                                        m_lastSummaryData.lastCycleFullVoltageHarmonics :
                                        m_lastSummaryData.lastCycleFullCurrentHarmonics;

    sources.harmonics = &fullHarmonics[phaseIndex];
    sources.totalRms = isVoltage ? &m_lastSummaryData.totalVoltageRms : &m_lastSummaryData.totalCurrentRms;
    sources.fundamental = isVoltage ? &m_lastSummaryData.fundamentalVoltage : &m_lastSummaryData.fundamentalCurrent;
    sources.dataTypeIndex = m_dataTypeComboBox->currentIndex();

    return sources;
}

double AnalysisHarmonicPage::calculateRawValue(const HarmonicDataSources& sources, int order, int phaseIndex) const
{
    double rawValue = 0.0;

    auto it = std::find_if(sources.harmonics->begin(), sources.harmonics->end(), [order](const HarmonicAnalysisResult& h) {
        return h.order == order;
    });

    if(it != sources.harmonics->end()) {
        const auto& harmonic = *it;
        if(sources.dataTypeIndex == 0) { // voltage or current
            rawValue = harmonic.rms;
        } else if(sources.dataTypeIndex == 1) { // %RMS
            double totalRms = (phaseIndex == 0) ? sources.totalRms->a : (phaseIndex == 1) ?
                                                                        sources.totalRms->b : sources.totalRms->c;

            if(totalRms > 1e-9) {
                rawValue = (harmonic.rms / totalRms) * 100.0;
            }
        } else { // Fund
            const auto& fundamental = (*sources.fundamental)[phaseIndex];
            if(fundamental.rms > 1e-9) {
                rawValue = (harmonic.rms / fundamental.rms) * 100.0;
            }
        }
    }
    return rawValue;
}

QString AnalysisHarmonicPage::formatValue(double value) const
{
    QString formattedValue;

    if(value >= 100.0) {
        formattedValue = QString::number(value, 'f', 1);
    } else if(value >= 10.0) {
        formattedValue = QString::number(value, 'f', 2);
    } else if(value >= 1.0) {
        formattedValue = QString::number(value, 'f', 3);
    } else {
        formattedValue = QString::number(value, 'f', 4);
    }

    // 전체 4자리 넘지 안도록 자르기
    if(formattedValue.length() > 4 && formattedValue.contains('.')) {
        formattedValue = formattedValue.left(5);
    }

    return formattedValue;
}

void AnalysisHarmonicPage::updateGraph()
{
    if(!m_hasData) return;

    const bool isVoltage = m_voltageButton->isChecked();

    const QString fundUnit = isVoltage ? "V" : "A";
    for(auto& label : m_fundUnitLabels) {
        label->setText(fundUnit);
    }

    const auto* thdData = isVoltage ? &m_lastSummaryData.voltageThd : &m_lastSummaryData.currentThd;
    const auto* fundData = isVoltage ? &m_lastSummaryData.fundamentalVoltage : &m_lastSummaryData.fundamentalCurrent;


    // 데이터 타입 콤보박스 선택에 따라 값 계산 및 그래프 업데이트
    double maxVal = 0.0; // 자동 스케일링을 위한 최대값

    for(int i{0}; i < 3; ++i) { // A상, B상, C상
        if(i == 0)
            m_thdValueLabels[i]->setText(QString::number(thdData->a, 'f', 1));
        else if(i == 1)
            m_thdValueLabels[i]->setText(QString::number(thdData->b, 'f', 1));
        else
            m_thdValueLabels[i]->setText(QString::number(thdData->c, 'f', 1));

        m_fundValueLabels[i]->setText(QString::number((*fundData)[i].rms, 'f', 1));

        
        // 막대 그래프 데이터 업데이트
        // Phase 체크박스 상태 확인
        if(!m_isPhaseVisible[i]) {
            for(int order{0}; order <= 50; ++order) {
                if(order < m_barSets[i]->count()) {
                    m_barSets[i]->replace(order, 0.0);
                }
            }
            continue;
        }

        HarmonicDataSources sources = getCurrentDataSources(i);
        for(int order{0}; order <= 50; ++order) {
            double rawValue = calculateRawValue(sources, order, i);
            double displayValue = (sources.dataTypeIndex == 0) ?
                                      AnalysisUtils::scaleValue(rawValue, m_scaleUnit) : rawValue;

            if(order == 1  && !m_isFundVisible) {
                displayValue = 0.0;
            }

            if(order < m_barSets[i]->count()) {
                m_barSets[i]->replace(order, displayValue);
            }

            if(order == 1 && !m_isFundVisible) continue;
            if(order > 0) {
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

void AnalysisHarmonicPage::updateText()
{
    if(!m_hasData) return;

    // 1. 어떤 상(A,B,C)의 데이터를 보여줄 지 결정
    int phaseIndex = 0;
    auto checkedButton = m_phaseButtonGroup->checkedButton();
    if(checkedButton) {
        phaseIndex = m_phaseButtonGroup->id(checkedButton);
    }

    // 2. 어떤 종류(VorA, %RMS, %Fund)의 데이터를 보여줄지 결정
    HarmonicDataSources sources = getCurrentDataSources(phaseIndex);

    // 3. 0차부터 50차까지 루프돌며 테이블 업데이트
    for(int order{0}; order <= 50; ++order) {
        double rawValue = calculateRawValue(sources, order, phaseIndex);

        // 4. rawValue를 포맷에 맞게 QString으로 변환
        QString formattedValue = formatValue(rawValue);
        qDebug() << "formattedValue: " << formattedValue;
        // 5. 테이블의 해당 셀에 값 업데이트
        int row = order % 9;
        int col = order / 9;
        QTableWidgetItem* item = m_textTable->item(row, col * 2 + 1);
        if(item) {
            item->setText(formattedValue);
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
    m_phaseButtonGroup = new QButtonGroup(this);
    m_phaseButtonGroup->setExclusive(false);

    for(int i{0}; i < 3; ++i) {
        m_phaseCheckBoxes[i] = new QCheckBox(phaseNames[i]);
        m_phaseCheckBoxes[i]->setChecked(true);
        m_phaseCheckBoxes[i]->setProperty("checkType", "phaseCheck");
        QString phaseStyle = QString("QCheckBox::checked { background-color: %1; }")
                                 .arg(config::View::PhaseColors::Voltage[i].name());
        m_phaseCheckBoxes[i]->setStyleSheet(phaseStyle);

        m_phaseButtonGroup->addButton(m_phaseCheckBoxes[i], i);
        controlBarLayout->addWidget(m_phaseCheckBoxes[i]);
    }

    connect(m_fundCheckBox, &QCheckBox::checkStateChanged, this, &AnalysisHarmonicPage::onFundVisibleChanged);
    connect(m_phaseButtonGroup, &QButtonGroup::idToggled, this, &AnalysisHarmonicPage::onPhaseVisibleChanged);
}

QWidget* AnalysisHarmonicPage::createGraphView()
{
    auto graphViewWidget = new QWidget();
    auto mainLayout = new QHBoxLayout(graphViewWidget);
    mainLayout->setContentsMargins(0, 5, 0, 0); // 컨트롤 바와의 간격

    auto leftPanelLayout = new QVBoxLayout();
    leftPanelLayout->addSpacing(40);

    // 1. 왼쪽 스케일 패널
    auto scaleBox = new QGroupBox("Scale");
    scaleBox->setObjectName("scaleBox");
    // scaleBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    scaleBox->setMaximumWidth(55);

    auto scaleLayout = new QVBoxLayout(scaleBox);
    scaleLayout->setContentsMargins(2, 15, 2, 2);

    m_scaleButtonGroup = new QButtonGroup(this);
    m_scaleButtonGroup->setExclusive(false);

    const QStringList scaleButtonNames = {"Auto", "+", "-"};
    for(int i{0}; i < 3; ++i) {
        m_scaleButtons[i] = new QPushButton(scaleButtonNames[i]);
        m_scaleButtons[i]->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        if(i > 0) {
            m_scaleButtons[i]->setProperty("buttonType", "waveformControl");
        }
        scaleLayout->addWidget(m_scaleButtons[i]);
        m_scaleButtonGroup->addButton(m_scaleButtons[i], 1);
    }

    m_scaleButtons[0]->setCheckable(true);
    m_scaleButtons[0]->setChecked(m_isAutoScaling);
    m_scaleButtons[0]->setProperty("buttonType", "waveformToggle");

    connect(m_scaleButtons[0], &QPushButton::toggled, this, &AnalysisHarmonicPage::onScaleAutoToggled);
    connect(m_scaleButtons[1], &QPushButton::clicked, this, &AnalysisHarmonicPage::onScaleInClicked);
    connect(m_scaleButtons[2], &QPushButton::clicked, this, &AnalysisHarmonicPage::onScaleOutClicked);

    leftPanelLayout->addWidget(scaleBox);

    mainLayout->addLayout(leftPanelLayout);

    // 2. 오른쪽 그래프 및 정보 패널
    auto rightPanel = new QWidget();
    auto rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(10, 0, 0, 0);

    // 2-1. 상단 정보 영역 (THD, Fund.)
    auto infoContainer = new QWidget();
    auto infoLayout = new QGridLayout(infoContainer);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setVerticalSpacing(0);
    // infoLayout->setSpacing(5);

    const QStringList phaseNames = {"A", "B", "C"};

    // THD 라인
    QLabel* thdLabel = new QLabel("THD");
    thdLabel->setProperty("harmonicLabelType", "title");
    thdLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLayout->addWidget(thdLabel, 0, 0);
    for(int i{0}; i < 3; ++i) {
        int col = 1 + (i * 3); // A, B, C 각 그룹의 시작 컬럼
        auto phaseLabel = new QLabel(phaseNames[i]);
        phaseLabel->setProperty("harmonicLabelType", "phase");
        QString phaseStyle = QString("color: %1;").arg(config::View::PhaseColors::Voltage[i].name());
        phaseLabel->setStyleSheet(phaseStyle);
        infoLayout->addWidget(phaseLabel, 0, col);

        m_thdValueLabels[i] = new QLabel("0.00");
        m_thdValueLabels[i]->setProperty("harmonicLabelType", "value");
        m_thdValueLabels[i]->setFixedWidth(30);
        infoLayout->addWidget(m_thdValueLabels[i], 0, col + 1);

        auto unitLabel = new QLabel("%");
        unitLabel->setProperty("harmonicLabelType", "unit");
        infoLayout->addWidget(unitLabel, 0, col + 2);
    }

    // Fund. 라인
    QLabel* fundLabel = new QLabel("Fund.");
    fundLabel->setProperty("harmonicLabelType", "title");
    fundLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLayout->addWidget(fundLabel, 1, 0);
    for(int i{0}; i < 3; ++i) {
        int col = 1 + (i * 3); // A, B, C 각 그룹의 시작 컬럼

        auto phaseLabel = new QLabel(phaseNames[i]);
        phaseLabel->setProperty("harmonicLabelType", "phase");
        QString phaseStyle = QString("color: %1;").arg(config::View::PhaseColors::Voltage[i].name());
        phaseLabel->setStyleSheet(phaseStyle);
        infoLayout->addWidget(phaseLabel, 1, col);

        m_fundValueLabels[i] = new QLabel("0.00");
        m_fundValueLabels[i]->setProperty("harmonicLabelType", "value");
        m_fundValueLabels[i]->setFixedWidth(30);
        infoLayout->addWidget(m_fundValueLabels[i], 1, col + 1);

        m_fundUnitLabels[i] = new QLabel("V");
        m_fundUnitLabels[i]->setProperty("harmonicLabelType", "unit");
        infoLayout->addWidget(m_fundUnitLabels[i], 1, col + 2);
    }

    infoLayout->setColumnStretch(0, 1);

    rightLayout->addWidget(infoContainer);

    // 2-2. 하단 막대 그래프
    m_chart = new QChart();
    m_chart->legend()->hide();
    m_chart->setPlotArea(QRect(25, 25, 335, 130));
    m_chart->setContentsMargins(-20, -10, -20, -10);

    m_chartView = new QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setBackgroundBrush(QBrush(Qt::white));

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
    m_axisX->setRange(-0.5, 50.5); // 0부터 50까지 모두 보이도록 설정
    m_axisX->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);

    m_axisX->append("0", 0);
    for(int i{1}; i <= 5; ++i)
        m_axisX->append(QString("%1").arg(8 * i), 8 * i);
    m_axisX->append("50", 50);

    m_axisX->setGridLineVisible(false); // 세로 그리드 라인 숨기기
    m_axisX->setTickCount(7);

    m_chart->addAxis(m_axisX, Qt::AlignBottom);

    // 막대 시리즈 및 세트 생성
    m_barSeries = new QBarSeries();
    m_barSeries->setBarWidth(1.0);
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
    m_textTable = new QTableWidget();
    m_textTable->setObjectName("harmonicsTextTable");
    m_textTable->setRowCount(9);
    m_textTable->setColumnCount(12);

    m_textTable->horizontalHeader()->hide();
    m_textTable->verticalHeader()->hide();
    m_textTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_textTable->setFocusPolicy(Qt::NoFocus);
    m_textTable->setAlternatingRowColors(false); // 기본 교차색상 끄기
    m_textTable->setShowGrid(true);
    m_textTable->setContentsMargins(0, 0, 0, 0);
    m_textTable->setItemDelegate(new HarmonicTextDelegate(m_textTable));
    m_textTable->setSelectionMode(QAbstractItemView::ExtendedSelection);

    const QColor orderBackgroundColor = QColor(211, 211, 211);
    const QFont textFont("Arial", 8);
    const QFont textFontBold("Arial", 8, QFont::Bold);

    for(int row{0}; row < 9; ++row) {
        for(int col{0}; col < 6; ++col) {
            int order = col * 9 + row;

            if(order > 50) {
                continue;
            }

            // 번호 칸(짝수 열)
            QTableWidgetItem* orderItem = new QTableWidgetItem(QString::number(order));
            orderItem->setTextAlignment(Qt::AlignCenter);
            orderItem->setFont(textFontBold);
            m_textTable->setItem(row, col * 2, orderItem);

            // 값 칸(홀수 열)
            QTableWidgetItem* valueItem = new QTableWidgetItem("0.000");
            valueItem->setTextAlignment(Qt::AlignCenter);
            valueItem->setFont(textFont);
            m_textTable->setItem(row, col * 2 + 1, valueItem);
        }
    }

    for (int col = 0; col < m_textTable->columnCount(); ++col) {
        if (col % 2 == 0)
            m_textTable->setColumnWidth(col, 20); // 번호
        else
            m_textTable->setColumnWidth(col, 50); // 값
    }

    // m_textTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_textTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_textTable->setContentsMargins(0, 0, 0, 0);

    return m_textTable;
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
    updateText();
    qDebug() << "-----------------------------------";

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
    updateText();
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
        m_scaleButtons[0]->setChecked(false);
        updateChartAxis();
        updateGraph();
    }
}

void AnalysisHarmonicPage::onScaleOutClicked()
{
    if(m_scaleIndex > 0) {
        --m_scaleIndex;
        m_scaleButtons[0]->setChecked(false);
        updateChartAxis();
        updateGraph();
    }
}

void AnalysisHarmonicPage::onFundVisibleChanged(bool checked)
{
    m_isFundVisible = checked;
    updateGraph();
}

void AnalysisHarmonicPage::onPhaseVisibleChanged(int id, bool checked)
{
    if(id >= 0 && id < 3) {
        m_isPhaseVisible[id] = checked;
        updateGraph();
        updateText();
    }
}

void AnalysisHarmonicPage::onViewTypeChanged(int index)
{
    if(index == 1) {
        m_fundCheckBox->setVisible(false);
        m_phaseCheckBoxes[0]->setChecked(false);
        m_phaseCheckBoxes[1]->setChecked(false);
        m_phaseCheckBoxes[2]->setChecked(false);
        m_phaseButtonGroup->setExclusive(true);

        // 상호 배타적이 되면, 첫 번째 버튼을 선택
        if(m_phaseButtonGroup->checkedButton() == nullptr) {
            m_phaseCheckBoxes[0]->setChecked(true);
        }
    } else { // Graph 모드
        m_fundCheckBox->setVisible(true);
        m_phaseButtonGroup->setExclusive(false);
    }
}

// =====================================================
HarmonicTextDelegate::HarmonicTextDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{

}

void HarmonicTextDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    QString text = index.model()->data(index, Qt::DisplayRole).toString();
    bool isSelected = option.state & QStyle::State_Selected;
    bool isOrderColumn = index.column() % 2 == 0;

    // 배경색 및 텍스트 설정
    if(isSelected) {
        painter->fillRect(option.rect, option.palette.highlight());
        painter->setPen(option.palette.highlightedText().color());
    } else if(isOrderColumn) {
        painter->fillRect(option.rect, QColor(211, 211, 211));
        painter->setPen(Qt::black);
    } else {
        painter->fillRect(option.rect, Qt::white);
        painter->setPen(Qt::black);
    }

    // 텍스트 그리기
    painter->drawText(option.rect, Qt::AlignCenter, text);

    painter->restore();
}

