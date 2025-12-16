#include "UIutils.h"
#include "analysis_utils.h"
#include "analysis_waveform_page.h"
#include <QButtonGroup>
#include <QChart>
#include <QChartView>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineSeries>
#include <QPushButton>
#include <QVBoxLayout>
#include <QValueAxis>

AnalysisWaveformPage::AnalysisWaveformPage(QWidget* parent)
    : QWidget(parent)
    , m_isUpdating(true)
    , m_isAutoScaling(true)
    , m_isTargetVoltage(true)
{
    setupUi();
}

void AnalysisWaveformPage::setupUi()
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);
    mainLayout->setSpacing(2);

    // 1. 상단 UI (제목, 전체 on/off)
    setupTopBar(mainLayout);

    // 2. 구분선
    QFrame* titleLine = new QFrame(this);
    titleLine->setFrameShape(QFrame::HLine);
    titleLine->setObjectName("titleLine");
    titleLine->setFixedHeight(2);
    mainLayout->addWidget(titleLine);

    // 3. 컨텐츠 영역
    auto middleLayout = new QHBoxLayout();
    mainLayout->addLayout(middleLayout, 1);

    QWidget* leftPanel = setupLeftPanel();
    QWidget* rightPanel = setupRightPanel();

    middleLayout->addWidget(leftPanel);
    middleLayout->addWidget(rightPanel, 1);


    for(int i{0}; i < 3; ++i) {
        connect(m_voltagePhaseChecks[i], &QCheckBox::toggled, m_voltageSeries[i], &QLineSeries::setVisible);
        connect(m_currentPhaseChecks[i], &QCheckBox::toggled, m_currentSeries[i], &QLineSeries::setVisible);
    }

    // 시작/정지 버튼 연결
    connect(m_startButton, &QPushButton::toggled, this, &AnalysisWaveformPage::onStartStopToggled);
    connect(m_scaleButtons[0], &QPushButton::toggled, this, &AnalysisWaveformPage::onScaleAutoToggled);
    connect(m_scaleButtons[1], &QPushButton::toggled, this, &AnalysisWaveformPage::onScaleTargetToggled);
    connect(m_scaleButtons[2], &QPushButton::clicked, this, &AnalysisWaveformPage::onScaleInClicked);
    connect(m_scaleButtons[3], &QPushButton::clicked, this, &AnalysisWaveformPage::onScaleOutClicked);

    // 초기 상태 설정
    onStartStopToggled(true);
    onScaleAutoToggled(true);
    onScaleTargetToggled(true);
}

void AnalysisWaveformPage::setupTopBar(QVBoxLayout* mainLayout)
{
    auto topLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel("Waveform");
    titleLabel->setObjectName("titleLabel");
    topLayout->addWidget(titleLabel);
    topLayout->addStretch();

    auto *voltageAllCheck = new QCheckBox("Voltage");
    auto *currentAllCheck = new QCheckBox("Current");
    voltageAllCheck->setChecked(true);
    currentAllCheck->setChecked(true);
    voltageAllCheck->setProperty("checkType", "allCheck");
    currentAllCheck->setProperty("checkType", "allCheck");
    topLayout->addWidget(voltageAllCheck);
    topLayout->addWidget(currentAllCheck);
    mainLayout->addLayout(topLayout);

    // All체크박스와 Waveform 가시성 연결
    connect(voltageAllCheck, &QCheckBox::toggled, this, [this](bool checked) {
        for(int i{0}; i < 3; ++i) {
            m_voltagePhaseChecks[i]->setChecked(checked);
            m_voltagePhaseChecks[i]->setEnabled(checked);
        }
        !checked ? m_axisV->setLabelsColor(Qt::transparent) : m_axisV->setLabelsColor(Qt::black);
    });
    connect(currentAllCheck, &QCheckBox::toggled, this, [this](bool checked) {
        for(int i{0}; i < 3; ++i) {
            m_currentPhaseChecks[i]->setChecked(checked);
            m_currentPhaseChecks[i]->setEnabled(checked);
        }
        !checked ? m_axisA->setLabelsColor(Qt::transparent) : m_axisA->setLabelsColor(Qt::black);
    });
}

QWidget* AnalysisWaveformPage::setupLeftPanel()
{
    auto leftButtonPanel = new QWidget();
    auto leftButtonLayout = new QVBoxLayout(leftButtonPanel);
    leftButtonLayout->setContentsMargins(0, 0, 0, 0);
    // leftButtonLayout->setSpacing(10); // 버튼 사이 간격

    m_startButton = new QPushButton("❚❚");
    m_startButton->setCheckable(true);
    m_startButton->setChecked(true);
    m_startButton->setProperty("buttonType", "waveformToggle");
    leftButtonLayout->addWidget(m_startButton);

    auto scaleBox = new QGroupBox("Scale");
    scaleBox->setObjectName("scaleBox");
    scaleBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    auto scaleButtonsLayout = new QVBoxLayout(scaleBox);
    scaleButtonsLayout->setContentsMargins(2, 2, 2, 2);

    m_scaleButtonGroup = new QButtonGroup(this);
    m_scaleButtonGroup->setExclusive(false);

    const QStringList scaleButtonNames = {"Auto", "V/A", "+", "-"};
    for(int i{0}; i < 4; ++i) {
        m_scaleButtons[i] = new QPushButton(scaleButtonNames[i]);
        m_scaleButtons[i]->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        if(i < 2) {
            m_scaleButtons[i]->setCheckable(true);
            m_scaleButtons[i]->setProperty("buttonType", "waveformToggle");

            if(i == 1) {// V/A버튼
                m_scaleButtons[i]->setProperty("toggleType", "VA");
            }
        } else {
            m_scaleButtons[i]->setProperty("buttonType", "waveformControl");
        }
        scaleButtonsLayout->addWidget(m_scaleButtons[i]);
        m_scaleButtonGroup->addButton(m_scaleButtons[i], 1);
    }
    m_scaleButtons[0]->setChecked(true); // Auto
    m_scaleButtons[1]->setChecked(true); // V가 기본
    leftButtonLayout->addWidget(scaleBox);

    return leftButtonPanel;
}

QWidget* AnalysisWaveformPage::setupRightPanel()
{
    auto rightContentPanel = new QWidget();
    auto rightContentLayout = new QVBoxLayout(rightContentPanel);
    rightContentLayout->setContentsMargins(10, 0, 0, 0);

    auto controlBarLayout = new QHBoxLayout();
    controlBarLayout->setContentsMargins(0, 5, 0, 0);
    controlBarLayout->setSpacing(2);

    QLabel* voltageLabel = new QLabel("Volt");
    QLabel* currentLabel = new QLabel("Curr");
    voltageLabel->setObjectName("waveformLabel");
    currentLabel->setObjectName("waveformLabel");

    controlBarLayout->addWidget(voltageLabel);
    for(int i{0}; i < 3; ++i) {
        m_voltagePhaseChecks[i] = new QCheckBox(QString(QChar('A' + i)));
        m_voltagePhaseChecks[i]->setChecked(true);

        m_voltagePhaseChecks[i]->setProperty("checkType", "phaseCheck");
        QString voltageStyle = QString("QCheckBox:checked { background-color: %1; }").arg(View::PhaseColors::Voltage[i].name());
        m_voltagePhaseChecks[i]->setStyleSheet(voltageStyle);

        controlBarLayout->addWidget(m_voltagePhaseChecks[i]);
    }
    controlBarLayout->addStretch(); // Volt와 Curr 사이에 최대한 공간을 만듦

    controlBarLayout->addWidget(currentLabel);
    for(int i{0}; i < 3; ++i) {
        m_currentPhaseChecks[i] = new QCheckBox(QString(QChar('A' + i)));
        m_currentPhaseChecks[i]->setChecked(true);

        m_currentPhaseChecks[i]->setProperty("checkType", "phaseCheck");
        QString currentStyle = QString("QCheckBox:checked { background-color: %1; }").arg(View::PhaseColors::Current[i].name());
        m_currentPhaseChecks[i]->setStyleSheet(currentStyle);

        controlBarLayout->addWidget(m_currentPhaseChecks[i]);
    }
    rightContentLayout->addLayout(controlBarLayout);
    // rightContentLayout->addSpacing();

    setupChart();
    rightContentLayout->addWidget(m_chartView, 1);

    return rightContentPanel;
}

void AnalysisWaveformPage::setupChart()
{
    m_chart = new QChart();
    m_chartView = new QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setBackgroundBrush(QBrush(Qt::white));

    QFont axisFont;
    axisFont.setPixelSize(8);

    auto* internalVLayout = new QVBoxLayout(m_chartView);
    auto* chartHeaderLayout = new QHBoxLayout();
    internalVLayout->addLayout(chartHeaderLayout);
    internalVLayout->addStretch(); // 헤더를 상단에 고정

    m_voltageScaleLabel = new QLabel("[V]");
    m_voltageScaleLabel->setFont(axisFont);
    m_voltageScaleLabel->setObjectName("axisUnitLabel");

    m_currentScaleLabel = new QLabel("[A]");
    m_currentScaleLabel->setFont(axisFont);
    m_currentScaleLabel->setObjectName("axisUnitLabel");

    chartHeaderLayout->addWidget(m_voltageScaleLabel);
    chartHeaderLayout->addStretch();
    chartHeaderLayout->addWidget(m_currentScaleLabel);
    chartHeaderLayout->setContentsMargins(15, 0, 15, 0);

    // 차트 및 축 설정
    m_chart->legend()->hide();
    m_chart->setPlotArea(QRect(25, 25, 325, 180));
    m_chart->setContentsMargins(-20, -10, -20, -10); // 여백 최소화

    m_axisX = new QValueAxis();
    m_axisX->setLabelsVisible(false);
    m_axisX->setTickCount(11);
    m_chart->addAxis(m_axisX, Qt::AlignBottom);

    m_axisV = new QValueAxis();
    m_axisV->setRange(-400, 400);
    m_axisV->setTickCount(9);
    m_axisV->setLabelFormat("%.1f");
    m_axisV->setLabelsFont(axisFont);
    m_chart->addAxis(m_axisV, Qt::AlignLeft);

    m_axisA = new QValueAxis();
    m_axisA->setRange(-400, 400);
    m_axisA->setTickCount(9);
    m_axisA->setLabelFormat("%.1f");
    m_axisA->setLabelsFont(axisFont);
    m_chart->addAxis(m_axisA, Qt::AlignRight);

    // 시리즈 생성 및 연결
    for(int i{0}; i < 3; ++i) {
        m_voltageSeries[i] = new QLineSeries();
        QPen voltagePen(View::PhaseColors::Voltage[i]);
        voltagePen.setWidth(1);
        m_voltageSeries[i]->setPen(voltagePen);
        m_chart->addSeries(m_voltageSeries[i]);
        m_voltageSeries[i]->attachAxis(m_axisX);
        m_voltageSeries[i]->attachAxis(m_axisV);

        m_currentSeries[i] = new QLineSeries();
        QPen currentPen(View::PhaseColors::Current[i]);
        currentPen.setWidth(1);
        m_currentSeries[i]->setPen(currentPen);
        m_chart->addSeries(m_currentSeries[i]);
        m_currentSeries[i]->attachAxis(m_axisX);
        m_currentSeries[i]->attachAxis(m_axisA);
    }
}

void AnalysisWaveformPage::onOneSecondDataUpdated(const OneSecondSummaryData& data)
{
    m_lastData = data;
    if(m_isUpdating) {
        updatePage();
    }
}

void AnalysisWaveformPage::updatePage()
{
    if(m_lastData.lastTwoCycleData.empty())
        return;

    if(m_isAutoScaling) {
        double v_abs_max = 0.0, a_abs_max = 0.0;

        for(const auto& point : m_lastData.lastTwoCycleData) {
            v_abs_max = std::max({v_abs_max, std::abs(point.voltage.a), std::abs(point.voltage.b), std::abs(point.voltage.c)});
            a_abs_max = std::max({a_abs_max, std::abs(point.current.a), std::abs(point.current.b), std::abs(point.current.c)});
        }

        // voltage auto scaling
        for(size_t i{0}; i < View::RANGE_TABLE.size(); ++i) {
            if(v_abs_max <= View::RANGE_TABLE[i]) {
                if(i != m_voltageScaleIndex) {
                    m_voltageScaleIndex = i;
                    updateAxis(true);
                }
                break;
            }
        }

        // current auto scaling
        for(size_t i{0}; i < View::RANGE_TABLE.size(); ++i) {
            if(a_abs_max <= View::RANGE_TABLE[i]) {
                if(i != m_currentScaleIndex) {
                    m_currentScaleIndex=  i;
                    updateAxis(false);
                }
                break;
            }
        }
    }

    const auto& waveData = m_lastData.lastTwoCycleData;

    // 데이터 포인트들을 각 시리즈에 맞게 분리

    // 데이터의 마지막 N개(2 사이클 분량)를 가져옴
    std::array<QList<QPointF>, 3> vPoints, iPoints;
    for(int i{0}; i < 3; ++i) {
        vPoints[i].reserve(waveData.size());
        iPoints[i].reserve(waveData.size());
    }

    double minV = std::numeric_limits<double>::max();
    double maxV = std::numeric_limits<double>::lowest();
    double minA = std::numeric_limits<double>::max();
    double maxA = std::numeric_limits<double>::lowest();

    for (const auto& point : waveData) {
        double timeSec = std::chrono::duration<double>(point.timestamp).count();
        const auto& v = point.voltage;
        const auto& i = point.current;

        vPoints[0].append(QPointF(timeSec, UIutils::scaleValue(v.a, m_voltageUnit)));
        vPoints[1].append(QPointF(timeSec, UIutils::scaleValue(v.b, m_voltageUnit)));
        vPoints[2].append(QPointF(timeSec, UIutils::scaleValue(v.c, m_voltageUnit)));

        iPoints[0].append(QPointF(timeSec, UIutils::scaleValue(i.a, m_currentUnit)));
        iPoints[1].append(QPointF(timeSec, UIutils::scaleValue(i.b, m_currentUnit)));
        iPoints[2].append(QPointF(timeSec, UIutils::scaleValue(i.c, m_currentUnit)));

        minV = std::min({minV, UIutils::scaleValue(v.a, m_voltageUnit), UIutils::scaleValue(v.b, m_voltageUnit), UIutils::scaleValue(v.c, m_voltageUnit)});
        maxV = std::max({maxV, UIutils::scaleValue(v.a, m_voltageUnit), UIutils::scaleValue(v.b, m_voltageUnit), UIutils::scaleValue(v.c, m_voltageUnit)});
        minA = std::min({minA, UIutils::scaleValue(i.a, m_currentUnit), UIutils::scaleValue(i.b, m_currentUnit), UIutils::scaleValue(i.c, m_currentUnit)});
        maxA = std::max({maxA, UIutils::scaleValue(i.a, m_currentUnit), UIutils::scaleValue(i.b, m_currentUnit), UIutils::scaleValue(i.c, m_currentUnit)});
    }

    // 시리즈 및 축 업데이트
    for(int i{0}; i < 3; ++i) {
        m_voltageSeries[i]->replace(vPoints[i]);
        m_currentSeries[i]->replace(iPoints[i]);
    }

    // 축 범위 설정
    if (!waveData.empty()) {
        m_axisX->setRange(vPoints[0].first().x(), vPoints[0].last().x());
    }
}

void AnalysisWaveformPage::onStartStopToggled(bool checked)
{
    m_isUpdating = checked;
    m_startButton->setText(checked ? "❚❚" : "▶");
}

void AnalysisWaveformPage::onScaleAutoToggled(bool checked)
{
    m_isAutoScaling = checked;
}

void AnalysisWaveformPage::onScaleTargetToggled(bool checked)
{
    if(checked) {
        m_isTargetVoltage = true;
        m_scaleButtons[1]->setText("V/a");
    } else {
        m_isTargetVoltage = false;
        m_scaleButtons[1]->setText("v/A");
    }
}

void AnalysisWaveformPage::onScaleInClicked()
{
    applyScaleStep(true, m_isTargetVoltage);
}

void AnalysisWaveformPage::onScaleOutClicked()
{
    applyScaleStep(false, m_isTargetVoltage);
}

void AnalysisWaveformPage::applyScaleStep(bool zoomIn, bool isVoltage)
{
    if(m_isAutoScaling)
        m_scaleButtons[0]->setChecked(false);

    int& index =(isVoltage) ? m_voltageScaleIndex : m_currentScaleIndex; // 멤버 인덱스 값 변경 가능

    if(zoomIn && index < (int)View::RANGE_TABLE.size() - 1)
        ++index;
    else if(!zoomIn && index > 0)
        --index;

    updateAxis(isVoltage);
    updatePage();
}

void AnalysisWaveformPage::updateAxis(bool isVoltageAxis)
{
    int& index = isVoltageAxis ? m_voltageScaleIndex : m_currentScaleIndex;
    ScaleUnit& unit = isVoltageAxis ? m_voltageUnit : m_currentUnit;
    QValueAxis* targetAxis = isVoltageAxis ? m_axisV : m_axisA;
    QLabel* targetLabel = isVoltageAxis ? m_voltageScaleLabel : m_currentScaleLabel;

    unit = updateUnit(targetAxis, targetLabel, index, isVoltageAxis);
}

ScaleUnit AnalysisWaveformPage::updateUnit(QValueAxis* axis, QLabel* label, int scaleIndex, bool isVoltage)
{
    if(!axis || !label) return ScaleUnit::Base;

    double newRange = View::RANGE_TABLE[scaleIndex];
    ScaleUnit unit = UIutils::updateScaleUnit(newRange);

    double displayRange = UIutils::scaleValue(newRange, unit);
    axis->setRange(-displayRange, displayRange);

    const char* baseUnit = isVoltage ? "V" : "A";
    QString unitString;
    if(unit == ScaleUnit::Milli)
        unitString = QString("m%1").arg(baseUnit);
    else if(unit == ScaleUnit::Kilo)
        unitString = QString("k%1").arg(baseUnit);
    else
        unitString = baseUnit;

    label->setText(QString("[%1]").arg(unitString));

    return unit;
}
