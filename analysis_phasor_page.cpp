#include "analysis_phasor_page.h"

#include "config.h"
#include "phasor_view.h"

#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>

AnalysisPhasorPage::AnalysisPhasorPage(QWidget *parent)
    : QWidget{parent}
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);

    // 상단(제목 + 체크박스)
    auto topLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel("Phasor [Vector Diagram]");
    titleLabel->setObjectName("titleLabel");
    topLayout->addWidget(titleLabel);
    topLayout->addStretch();
    auto* voltageCheck = new QCheckBox("Voltage");
    auto* currentCheck = new QCheckBox("Current");
    voltageCheck->setChecked(true);
    currentCheck->setChecked(true);
    voltageCheck->setProperty("checkType", "allCheck");
    currentCheck->setProperty("checkType", "allCheck");
    topLayout->addWidget(voltageCheck);
    topLayout->addWidget(currentCheck);
    mainLayout->addLayout(topLayout);

    // 상단 밑 진한 라인
    QFrame* titleLine = new QFrame(this);
    titleLine->setFrameShape(QFrame::HLine);
    titleLine->setObjectName("titleLine");
    titleLine->setFixedHeight(2);
    mainLayout->addWidget(titleLine);

    // 컨텐츠 (페이저 _ 데이터 테이블)
    auto contentLayout = new QHBoxLayout();

    // PhasorView 위젯 사용
    m_phasorView = new PhasorView(this);
    m_phasorView->setMinimumSize(180, 180);
    m_phasorView->setInfoLabelVisibility(false);

    // 처음에 모든 페이저 표시
    for(int i{0}; i < 6; ++i) {
        m_phasorView->onVisibilityChanged(i, true);
    }

    contentLayout->addWidget(m_phasorView, 1);


    // 데이터 테이블 (VLN)
    // GridLayout을 감쌀 컨테이너 위젯
    auto* gridContainer = new QWidget(this);
    auto tableLayout = new QGridLayout(gridContainer);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    tableLayout->setSpacing(0);

    // 전압 섹션
    m_voltageTableContainer = new QWidget(this);
    auto voltageSectionLayout = new QVBoxLayout(m_voltageTableContainer);
    voltageSectionLayout->setContentsMargins(0, 0, 0, 0);
    voltageSectionLayout->setSpacing(5);

    auto voltageData = createPhasorTable(voltageSectionLayout, "Voltage", {"A", "B", "C"}, "V");
    m_voltageNameLabels = voltageData.first;
    m_voltageTable = voltageData.second;
    tableLayout->addWidget(m_voltageTableContainer, 0, 0); // 0번 행

    // 전류 섹션
    m_currentTableContainer = new QWidget(this);
    auto currentSectionLayout = new QVBoxLayout(m_currentTableContainer);
    currentSectionLayout->setContentsMargins(0, 0, 0, 0);
    currentSectionLayout->setSpacing(0);

    auto currentData = createPhasorTable(currentSectionLayout, "Current", {"A", "B", "C"}, "A");
    m_currentNameLabels = currentData.first;
    m_currentTable = currentData.second;
    tableLayout->addWidget(m_currentTableContainer, 2, 0); // 2번 행

    // 스페이서 위젯 추가
    auto spacerWidget = new QWidget(this);
    tableLayout->addWidget(spacerWidget, 1, 0); // 1번 행

    // 행 스트레치 비율 설정
    tableLayout->setRowStretch(0, 0); // 전압 섹션은 늘어나지 않음
    tableLayout->setRowStretch(1, 1); // 스페이서 위젯이 남은 모든 공간 차지
    tableLayout->setRowStretch(2, 0); // 전류 섹션은 늘어나지 않음

    contentLayout->addWidget(gridContainer, 0, Qt::AlignCenter);
    mainLayout->addLayout(contentLayout, 1);

    for(int i{0}; i < 3; ++i) {
        QString voltageStyle = QString("background-color: %1; color: white;").arg(config::View::PhaseColors::Voltage[i].name());
        m_voltageNameLabels[i]->setStyleSheet(voltageStyle);

        QString currentStyle = QString("background-color: %1; color: white;").arg(config::View::PhaseColors::Current[i].name());
        m_currentNameLabels[i]->setStyleSheet(currentStyle);
    }

    // 체크박스와 PhasorView 가시성 연결
    connect(voltageCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_phasorView->onVisibilityChanged(0, checked); // V(A)
        m_phasorView->onVisibilityChanged(1, checked); // V(B)
        m_phasorView->onVisibilityChanged(2, checked); // V(C)
        m_voltageTableContainer->setVisible(checked);
    });
    connect(currentCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_phasorView->onVisibilityChanged(3, checked); // I(A)
        m_phasorView->onVisibilityChanged(4, checked); // I(B)
        m_phasorView->onVisibilityChanged(5, checked); // I(C)
        m_currentTableContainer->setVisible(checked);
    });
}

void AnalysisPhasorPage::updateSummaryData(const OneSecondSummaryData& data)
{
    // PhasorView 업데이트
    m_phasorView->updateData(data.fundamentalVoltage, data.fundamentalCurrent, data.lastCycleVoltageHarmonics, data.lastCycleCurrentHarmonics);

    auto updateTable = [](auto& table, const auto& phaseData) {
        auto updatePhase = [&](int index, const auto& data) {
            table[index * 2 + 0]->setText(QString::number(data.rms, 'f', 3));
            table[index * 2 + 1]->setText(QString::number(utils::radiansToDegrees(data.phase), 'f', 1) + "°");
        };
        updatePhase(0, phaseData.a);
        updatePhase(1, phaseData.b);
        updatePhase(2, phaseData.c);
    };

    // Voltage
    updateTable(m_voltageTable, data.fundamentalVoltage);

    // Current
    updateTable(m_currentTable, data.fundamentalCurrent);
}

std::pair<std::array<QLabel*, 3>, std::array<QLabel*, 6>> AnalysisPhasorPage::createPhasorTable(QVBoxLayout* layout, const QString& title, const QStringList& labels, const QString& unit)
{
    // 제목
    QLabel* titleLabel = new QLabel(title, this);
    titleLabel->setObjectName("phasorTitleLabel");
    layout->addWidget(titleLabel);

    QFrame* hLine = new QFrame(this);
    hLine->setFrameShape(QFrame::HLine);
    hLine->setFixedHeight(1);
    hLine->setObjectName("phasorHLine");
    layout->addWidget(hLine);
    layout->addSpacing(3);

    std::array<QLabel*, 3> nameLabels;
    std::array<QLabel*, 6> valueLabels;
    for(int i{0}; i < 3; ++i) {
        auto rowLayout = new QHBoxLayout();
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(0);

        QLabel* nameLabel = new QLabel(labels[i]);
        nameLabel->setMinimumHeight(20);
        nameLabel->setMinimumWidth(20);
        nameLabel->setAlignment(Qt::AlignCenter);
        nameLabel->setObjectName("phasorNameLabel");
        rowLayout->addWidget(nameLabel, 0);
        nameLabels[i] = nameLabel; // 이름 라벨 포인터 저장

        QLabel* valueLabel = new QLabel("0.000", this);
        valueLabel->setFixedWidth(60);
        valueLabel->setObjectName("phasorValueLabel");

        QLabel* unitLabel = new QLabel(unit, this);
        unitLabel->setObjectName("phasorUnitLabel");

        QLabel* phaseLabel = new QLabel("0.0° ", this);
        phaseLabel->setFixedWidth(50);
        phaseLabel->setObjectName("phasorValueLabel");

        valueLabels[i * 2 + 0] = valueLabel; // 값
        valueLabels[i * 2 + 1] = phaseLabel; // 위상

        valueLabels[i * 2 + 0]->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        valueLabels[i * 2 + 1]->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        rowLayout->addWidget(valueLabels[i * 2 + 0], 1);
        rowLayout->addWidget(unitLabel);
        rowLayout->addWidget(valueLabels[i * 2 + 1], 1);

        layout->addLayout(rowLayout);
        layout->addSpacing(5);
    }
    return {nameLabels, valueLabels};
}
