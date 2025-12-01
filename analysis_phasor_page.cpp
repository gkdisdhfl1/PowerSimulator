#include "analysis_phasor_page.h"

#include "config.h"
#include "phasor_view.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

AnalysisPhasorPage::AnalysisPhasorPage(QWidget *parent)
    : QWidget{parent}
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);

    createTopBar(mainLayout);

    // 상단 밑 진한 라인
    QFrame* titleLine = new QFrame(this);
    titleLine->setFrameShape(QFrame::HLine);
    titleLine->setObjectName("titleLine");
    titleLine->setFixedHeight(1);
    titleLine->setStyleSheet("background:#444");
    mainLayout->addWidget(titleLine);

    // 컨텐츠 (페이저 _ 데이터 테이블)
    auto contentLayout = new QHBoxLayout();
    createContentArea(contentLayout);

    mainLayout->addLayout(contentLayout, 1);

    setupConnections();
}

void AnalysisPhasorPage::updateSummaryData(const OneSecondSummaryData& data)
{
    m_lastSummaryData = data;

    GenericPhaseData<HarmonicAnalysisResult> voltageData;
    QStringList voltageLabels;

    if(m_vllButton->isChecked()) { // VLL 모드
        // L-L 데이터 매핑
        voltageData.a = data.fundamentalVoltage_ll.ab;
        voltageData.b = data.fundamentalVoltage_ll.bc;
        voltageData.c = data.fundamentalVoltage_ll.ca;        
        voltageLabels = {"AB", "BC", "CA"};
    } else { // VLN 모드
        voltageData = data.fundamentalVoltage;
        voltageLabels = {"A", "B", "C"};
    }

    for(int i{0}; i < 3; ++i) {
        m_voltageNameLabels[i]->setText(voltageLabels[i]);
    }

    // PhasorView 업데이트
    m_phasorView->updateData(voltageData, data.fundamentalCurrent, data.lastCycleVoltageHarmonics.a, data.lastCycleCurrentHarmonics.a);

    // Voltage, Current table 업데이트
    updatePhasorTable(m_voltageTable, voltageData);
    updatePhasorTable(m_currentTable, data.fundamentalCurrent);
}

AnalysisPhasorPage::TableWidgets AnalysisPhasorPage::createPhasorTable(QVBoxLayout* layout, const QString& title, const QStringList& labels, const QString& unit)
{
    auto headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 3);

    // 제목
    QLabel* titleLabel = new QLabel(title, this);
    titleLabel->setObjectName("phasorTitleLabel");
    headerLayout->addWidget(titleLabel);

    layout->addLayout(headerLayout);

    QFrame* hLine = new QFrame(this);
    hLine->setFrameShape(QFrame::HLine);
    hLine->setFixedHeight(1);
    hLine->setObjectName("phasorHLine");
    layout->addWidget(hLine);
    layout->addSpacing(3);

    auto createLabel = [&](const QString& text, int fixedWidth, const QString& objName, Qt::Alignment alignment) {
        auto* label = new QLabel(text, this);
        if(fixedWidth > 0) label->setFixedWidth(fixedWidth);
        label->setObjectName(objName);
        label->setAlignment(alignment);
        return label;
    };

    std::array<QLabel*, 3> nameLabels;
    std::array<QLabel*, 6> valueLabels;

    for(int i{0}; i < 3; ++i) {
        auto rowLayout = new QHBoxLayout();
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(4);

        // Name Label
        QLabel* nameLabel = createLabel(labels[i], 0, "phasorNameLabel", Qt::AlignCenter);
        nameLabel->setMinimumHeight(20);
        nameLabel->setMinimumWidth(26);
        rowLayout->addWidget(nameLabel, 0);
        nameLabels[i] = nameLabel; // 이름 라벨 포인터 저장

        // Value Label
        QLabel* valueLabel = createLabel("0.000", 70, "phasorValueLabel", Qt::AlignLeft | Qt::AlignVCenter);
        valueLabels[i * 2 + 0] = valueLabel; // 값

        // Unit Label
        QLabel* unitLabel = createLabel(unit, 0, "phasorUnitLabel", Qt::AlignLeft | Qt::AlignVCenter);

        // Phase Label
        QLabel* phaseLabel = createLabel("0.0° ", 60, "phasorValueLabel", Qt::AlignRight | Qt::AlignVCenter);
        valueLabels[i * 2 + 1] = phaseLabel; // 위상

        // Layout 추가
        rowLayout->addWidget(valueLabels[i * 2 + 0], 1);
        rowLayout->addWidget(unitLabel);
        rowLayout->addWidget(valueLabels[i * 2 + 1], 1);

        layout->addLayout(rowLayout);
        layout->addSpacing(5);
    }
    return {nameLabels, valueLabels, headerLayout};
}

void AnalysisPhasorPage::createTopBar(QVBoxLayout* mainLayout)
{
    // 상단(제목 + 체크박스)
    auto topLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel("Phasor [Vector Diagram]");
    titleLabel->setObjectName("titleLabel");
    topLayout->addWidget(titleLabel);
    topLayout->addStretch();

    // Voltage 체크박스
    m_voltageCheck = new QCheckBox("Voltage");
    m_voltageCheck->setChecked(true);
    m_voltageCheck->setProperty("checkType", "allCheck");
    topLayout->addWidget(m_voltageCheck);

    // Current 체크박스
    m_currentCheck = new QCheckBox("Current");
    m_currentCheck->setChecked(true);
    m_currentCheck->setProperty("checkType", "allCheck");
    topLayout->addWidget(m_currentCheck);

    mainLayout->addLayout(topLayout);
}

void AnalysisPhasorPage::createContentArea(QHBoxLayout* contentLayout)
{
    // PhasorView 위젯 사용
    m_phasorView = new PhasorView(this);
    m_phasorView->setMinimumSize(170, 170);
    m_phasorView->setInfoLabelVisibility(false);

    // 처음에 모든 페이저 표시
    for(int i{0}; i < 6; ++i) {
        m_phasorView->onVisibilityChanged(i, true);
    }
    contentLayout->addWidget(m_phasorView);

    // GridLayout을 감쌀 컨테이너 위젯
    auto* gridContainer = new QWidget(this);
    auto tableLayout = new QVBoxLayout(gridContainer);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    tableLayout->setSpacing(5);

    createTableSection(tableLayout);

    contentLayout->addWidget(gridContainer, 0, Qt::AlignCenter);
}

void AnalysisPhasorPage::createTableSection(QVBoxLayout* tableLayout)
{
    // 전압 섹션
    m_voltageTableContainer = new QWidget(this);
    QSizePolicy spV = m_voltageTableContainer->sizePolicy();
    spV.setRetainSizeWhenHidden(true);
    m_voltageTableContainer->setSizePolicy(spV);

    auto voltageSectionLayout = new QVBoxLayout(m_voltageTableContainer);
    voltageSectionLayout->setContentsMargins(0, 0, 0, 0);
    voltageSectionLayout->setSpacing(0);

    // createPhasorTable 호출 및 반환값 받기
    auto voltageData = createPhasorTable(voltageSectionLayout, "Voltage", {"A", "B", "C"}, "V");
    m_voltageNameLabels = voltageData.nameLabels;
    m_voltageTable = voltageData.valueLabels;

    // Voltage 제목 줄에 토글 버튼 추가
    auto* voltageHeaderLayout = voltageData.headerLayout;
    voltageHeaderLayout->addStretch(); // 제목과 버튼 사이 간격

    m_vlnButton = new QPushButton("VLN");
    m_vlnButton->setCheckable(true);
    m_vlnButton->setChecked(true);
    m_vlnButton->setFixedWidth(50);
    m_vlnButton->setObjectName("phasorButton");

    m_vllButton = new QPushButton("VLL");
    m_vllButton->setCheckable(true);
    m_vllButton->setFixedWidth(50);
    m_vllButton->setObjectName("phasorButton");

    m_voltageModeGroup = new QButtonGroup(this);
    m_voltageModeGroup->addButton(m_vlnButton);
    m_voltageModeGroup->addButton(m_vllButton);
    m_voltageModeGroup->setExclusive(true);

    voltageHeaderLayout->addWidget(m_vlnButton);
    voltageHeaderLayout->addSpacing(3);
    voltageHeaderLayout->addWidget(m_vllButton);

    tableLayout->addWidget(m_voltageTableContainer);

    // 전류 섹션
    m_currentTableContainer = new QWidget(this);
    QSizePolicy spC = m_currentTableContainer->sizePolicy();
    spC.setRetainSizeWhenHidden(true);
    m_currentTableContainer->setSizePolicy(spC);

    auto currentSectionLayout = new QVBoxLayout(m_currentTableContainer);
    currentSectionLayout->setContentsMargins(0, 0, 0, 0);
    currentSectionLayout->setSpacing(0);

    auto currentData = createPhasorTable(currentSectionLayout, "Current", {"A", "B", "C"}, "A");
    m_currentNameLabels = currentData.nameLabels;
    m_currentTable = currentData.valueLabels;
    tableLayout->addWidget(m_currentTableContainer);

    for(int i{0}; i < 3; ++i) {
        QString voltageStyle = QString(
                                   "background-color: %1; ").arg(config::View::PhaseColors::Voltage[i].name());
        m_voltageNameLabels[i]->setStyleSheet(voltageStyle);

        QString currentStyle = QString(
                                   "background-color: %1;").arg(config::View::PhaseColors::Current[i].name());
        m_currentNameLabels[i]->setStyleSheet(currentStyle);
    }
}

void AnalysisPhasorPage::setupConnections()
{
    // 버튼 클릭 시 업데이트 연결
    connect(m_voltageModeGroup, &QButtonGroup::buttonClicked, this, [this] {
        updateSummaryData(m_lastSummaryData);
    });

    // 체크박스와 PhasorView 가시성 연결
    connect(m_voltageCheck, &QCheckBox::toggled, this, [this](bool checked) {
        for(int i{0}; i < 3; ++i) {
            m_phasorView->onVisibilityChanged(i, checked);
        }
        m_voltageTableContainer->setVisible(checked);
    });
    connect(m_currentCheck, &QCheckBox::toggled, this, [this](bool checked) {
        for(int i{3}; i < 6; ++i) {
            m_phasorView->onVisibilityChanged(i, checked);
        }
        m_currentTableContainer->setVisible(checked);
    });
}
