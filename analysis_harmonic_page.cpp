#include "analysis_harmonic_page.h"

#include <QButtonGroup>
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
#include <config.h>

AnalysisHarmonicPage::AnalysisHarmonicPage(QWidget *parent)
    : QWidget{parent}
    , m_voltageButton(nullptr)
    , m_currentButton(nullptr)
    , m_buttonGroup(nullptr)
{
    setupUi();
}

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

    // 뷰 타입 콤보박스와 스택 위젯 연결
    connect(m_viewTypeComboBox, &QComboBox::currentIndexChanged, m_contentStack, &QStackedWidget::setCurrentIndex);
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

    auto autoButton = new QPushButton("Auto");
    auto plusButton = new QPushButton("+");
    auto minusButton = new QPushButton("-");
    scaleLayout->addWidget(autoButton);
    scaleLayout->addWidget(plusButton);
    scaleLayout->addWidget(minusButton);
    // scaleLayout->addStretch();

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
    auto chart = new QChart();
    chart->setTitle("Harmonic Spectrum");
    chart->legend()->hide();
    auto chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    rightLayout->addWidget(chartView, 1);

    mainLayout->addWidget(rightPanel, 1); // 오른쪽 패널이 남은 공간 모두 차지

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
}
