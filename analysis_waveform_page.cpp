#include "analysis_waveform_page.h"
#include <QButtonGroup>
#include <QChart>
#include <QChartView>
#include <QCheckBox>
#include <QLabel>
#include <QLineSeries>
#include <QPushButton>
#include <QVBoxLayout>
#include <QValueAxis>

AnalysisWaveformPage::AnalysisWaveformPage(QWidget* parent)
    : QWidget(parent)
    , m_isUpdating(true)
    , m_axisTarget(AxisTarget::Voltage)
    , m_isAutoScaling(true)
{
    setupUi();
}

void AnalysisWaveformPage::setupUi()
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);
    mainLayout->setSpacing(2);

    // 1. 상단 UI (제목, 전체 on/off)
    auto topLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel("Waveform");
    titleLabel->setObjectName("titleLabel");
    topLayout->addWidget(titleLabel);
    topLayout->addStretch();

    auto* voltageAllCheck = new QCheckBox("Voltage");
    auto* currentAllCheck = new QCheckBox("Current");
    voltageAllCheck->setChecked(true);
    currentAllCheck->setChecked(true);
    voltageAllCheck->setObjectName("check");
    currentAllCheck->setObjectName("check");
    topLayout->addWidget(voltageAllCheck);
    topLayout->addWidget(currentAllCheck);
    mainLayout->addLayout(topLayout);

    // 2. 구분선
    QFrame* titleLine = new QFrame(this);
    titleLine->setFrameShape(QFrame::HLine);
    titleLine->setObjectName("titleLine");
    titleLine->setFixedHeight(2);
    mainLayout->addWidget(titleLine);

    // 3. 중간 컨트롤 바 (시작/정지, 상별 on/off)
    auto controlBarLayout = new QHBoxLayout();
    controlBarLayout->setContentsMargins(0, 0, 0, 0);
    controlBarLayout->setSpacing(2);
    m_startButton = new QPushButton("❚❚");
    m_startButton->setCheckable(true);
    m_startButton->setChecked(true);
    controlBarLayout->addWidget(m_startButton);
    controlBarLayout->addSpacing(20);

    QLabel* voltageLabel = new QLabel("Volt");
    QLabel* currentLabel = new QLabel("Curr");
    voltageLabel->setObjectName("waveformLabel");
    currentLabel->setObjectName("waveformLabel");

    controlBarLayout->addWidget(voltageLabel);
    for(int i{0}; i < 3; ++i) {
        m_voltagePhaseChecks[i] = new QCheckBox(QString(QChar('A' + i)));
        m_voltagePhaseChecks[i]->setChecked(true);
        controlBarLayout->addWidget(m_voltagePhaseChecks[i]);
    }
    controlBarLayout->addSpacing(20);

    controlBarLayout->addWidget(currentLabel);
    for(int i{0}; i < 3; ++i) {
        m_currentPhaseChecks[i] = new QCheckBox(QString(QChar('A' + i)));
        m_currentPhaseChecks[i]->setChecked(true);
        controlBarLayout->addWidget(m_currentPhaseChecks[i]);
    }
    controlBarLayout->addStretch();
    mainLayout->addLayout(controlBarLayout);

    // 4. 컨텐츠 영역 (스케일 버튼, 차트)
    auto contentLayout = new QHBoxLayout();

    // 4-1 스케일 버튼 그룹
    auto scaleButtonsContainer = new QWidget();
    scaleButtonsContainer->setObjectName("scaleButtonsContainer");
    scaleButtonsContainer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    auto scaleButtonsLayout = new QVBoxLayout(scaleButtonsContainer);
    // scaleButtonsLayout->setSpacing(10);
    // scaleButtonsLayout->setContentsMargins(5, 5, 5, 5);

    m_scaleButtonGroup = new QButtonGroup(this);
    m_scaleButtonGroup->setExclusive(false);

    const QStringList scaleButtonNames = {"Auto", "V/A", "+", "-"};
    for(int i{0}; i < 4; ++i) {
        m_scaleButtons[i] = new QPushButton(scaleButtonNames[i]);
        if(i < 2) {
            m_scaleButtons[i]->setCheckable(true);
        }
        m_scaleButtons[i]->setObjectName("waveformScaleButton");
        scaleButtonsLayout->addWidget(m_scaleButtons[i]);
        m_scaleButtonGroup->addButton(m_scaleButtons[i], 1);
    }
    m_scaleButtons[0]->setChecked(true); // Auto
    m_scaleButtons[1]->setChecked(true); // V가 기본
    scaleButtonsLayout->addStretch();
    contentLayout->addWidget(scaleButtonsContainer);

    // 4-2 차트 뷰
    m_chart = new QChart();
    m_chartView = new QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    contentLayout->addWidget(m_chartView, 1);

    // 차트 및 축 설정
    m_chart->legend()->hide();
    m_chart->setContentsMargins(-20, -10, -20, -10); // 여백 최소화

    QFont axisFont;
    axisFont.setPixelSize(8);

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
    m_axisA->setRange(-10, 10);
    m_axisA->setTickCount(9);
    m_axisA->setLabelFormat("%.1f");
    m_axisA->setLabelsFont(axisFont);
    m_chart->addAxis(m_axisA, Qt::AlignRight);

    // 시리즈 생성 및 연결
    const std::array<QColor, 3> voltageColors = {Qt::blue, Qt::darkYellow, Qt::black};
    const std::array<QColor, 3> currentColors = {Qt::red, QColorConstants::Svg::orange, QColorConstants::Svg::gray};

    mainLayout->addLayout(contentLayout, 1);

    for(int i{0}; i < 3; ++i) {
        m_voltageSeries[i] = new QLineSeries();
        m_voltageSeries[i]->setColor(voltageColors[i]);
        m_chart->addSeries(m_voltageSeries[i]);
        m_voltageSeries[i]->attachAxis(m_axisX);
        m_voltageSeries[i]->attachAxis(m_axisV);
        connect(m_voltagePhaseChecks[i], &QCheckBox::toggled, m_voltageSeries[i], &QLineSeries::setVisible);

        m_currentSeries[i] = new QLineSeries();
        m_currentSeries[i]->setColor(currentColors[i]);
        m_chart->addSeries(m_currentSeries[i]);
        m_currentSeries[i]->attachAxis(m_axisX);
        m_currentSeries[i]->attachAxis(m_axisA);
        connect(m_currentPhaseChecks[i], &QCheckBox::toggled, m_currentSeries[i], &QLineSeries::setVisible);
    }

    // 전체 on/off 체크박스 연결
    connect(voltageAllCheck, &QCheckBox::toggled, [this](bool checked){
        for(int i{0}; i < 3; ++i) m_voltagePhaseChecks[i]->setChecked(checked);
    });
    connect(currentAllCheck, &QCheckBox::toggled, [this](bool checked){
        for(int i{0}; i < 3; ++i) m_currentPhaseChecks[i]->setChecked(checked);
    });

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

void AnalysisWaveformPage::updateWaveformData(const OneSecondSummaryData& data)
{
    if(!m_isUpdating || data.lastTwoCycleData.empty())
        return;

    const auto& waveData = data.lastTwoCycleData;

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

        vPoints[0].append(QPointF(timeSec, v.a));
        vPoints[1].append(QPointF(timeSec, v.b));
        vPoints[2].append(QPointF(timeSec, v.c));

        iPoints[0].append(QPointF(timeSec, i.a));
        iPoints[1].append(QPointF(timeSec, i.b));
        iPoints[2].append(QPointF(timeSec, i.c));

        minV = std::min({minV, v.a, v.b, v.c});
        maxV = std::max({maxV, v.a, v.b, v.c});
        minA = std::min({minA, i.a, i.b, i.c});
        maxA = std::max({maxA, i.a, i.b, i.c});
    }

    // 시리즈 및 축 업데이트
    for(int i{0}; i < 3; ++i) {
        m_voltageSeries[i]->replace(vPoints[i]);
        m_currentSeries[i]->replace(iPoints[i]);
    }

    // 축 범위 설정
    if (!waveData.empty()) {
        m_axisX->setRange(vPoints[0].first().x(), vPoints[0].last().x());

        if(m_isAutoScaling) {
            double voltagePadding = (maxV - minV) * 0.1 + 1.0;
            m_axisV->setRange(minV - voltagePadding, maxV + voltagePadding);

            double amperePadding = (maxA - minA) * 0.1 + 1.0;
            m_axisA->setRange(minA - amperePadding, maxA + amperePadding);
        }
    }
}

void AnalysisWaveformPage::onStartStopToggled(bool checked)
{
    m_isUpdating = checked;
    m_startButton->setText(checked ? "❚❚" : "▶");
}

void AnalysisWaveformPage::onScaleAutoToggled(bool checked)
{
    qDebug() << "onScaleAutoToggled process";
    m_isAutoScaling = checked;
    qDebug() << "current m_isAutoScaling Value: " << m_isAutoScaling;
    qDebug() << "---------------------------";
}

void AnalysisWaveformPage::onScaleTargetToggled(bool checked)
{
    if(checked) {
        m_axisTarget = AxisTarget::Voltage;
        m_scaleButtons[1]->setText("V/a");
    } else {
        m_axisTarget = AxisTarget::Amperage;
        m_scaleButtons[1]->setText("v/A");
    }
}

void AnalysisWaveformPage::onScaleInClicked()
{
    qDebug() << "inClicked";
    if(m_isAutoScaling)
        m_scaleButtons[0]->setChecked(false);

    if(!m_isAutoScaling) {
        QValueAxis* targetAxis = (m_axisTarget == AxisTarget::Voltage) ? m_axisV : m_axisA;
        double center = (targetAxis->max() + targetAxis->min()) / 2.0;
        double span = (targetAxis->max() - targetAxis->min()) * 0.8;
        qDebug() << "m_axisV: " << m_axisV;
        qDebug() << "m_axisA: " << m_axisA;
        qDebug() << "targetAxis: " << targetAxis;
        qDebug() << "targetAixs -> setRange(" << center << " - " << span << " / 2.0, " << center << " + " << span << " / 2.0)";
        qDebug() << "---------------------------";
        targetAxis->setRange(center - span / 2.0, center + span / 2.0);
    }
}

void AnalysisWaveformPage::onScaleOutClicked()
{
    qDebug() << "outClicked";
    if(m_isAutoScaling)
        m_scaleButtons[0]->setChecked(false);

    if(!m_isAutoScaling) {
        QValueAxis* targetAxis = (m_axisTarget == AxisTarget::Voltage) ? m_axisV : m_axisA;
        double center = (targetAxis->max() + targetAxis->min()) / 2.0;
        double span = (targetAxis->max() - targetAxis->min()) * 1.2;
        qDebug() << "m_axisV: " << m_axisV;
        qDebug() << "m_axisA: " << m_axisA;
        qDebug() << "targetAxis: " << targetAxis;
        qDebug() << "targetAixs -> setRange(" << center << " - " << span << " / 2.0, " << center << " + " << span << " / 2.0)";
        qDebug() << "---------------------------";
        targetAxis->setRange(center - span / 2.0, center + span / 2.0);
    }
}
