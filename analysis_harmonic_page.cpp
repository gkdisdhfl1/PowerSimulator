#include "analysis_harmonic_page.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

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

    // 3. 컨트롤 바 추가
    auto controlBarLayout = new QHBoxLayout();
    mainLayout->addLayout(controlBarLayout);

    // 데이터 종류 콤보박스
    m_dataTypeComboBox = new QComboBox();
    m_dataTypeComboBox->addItems({"Voltage", "[%]RMS", "[%]Fund"});
    controlBarLayout->addWidget(m_dataTypeComboBox);

    // 표시 형식 콤보박스
    m_viewTypeComboBox = new QComboBox();
    m_viewTypeComboBox->addItems({"Graph", "Text"});
    controlBarLayout->addWidget(m_viewTypeComboBox);

    controlBarLayout->addSpacing(30);

    // Fund. 체크박스
    m_fundCheckBox = new QCheckBox("Fund.");
    m_fundCheckBox->setChecked(true);

    controlBarLayout->addWidget(m_fundCheckBox);
    controlBarLayout->addStretch();

    // A, B, C 상 체크박스
    const QStringList phaseNames = {"A", "B", "C"};
    for(int i{0}; i < 3; ++i) {
        m_phaseCheckBoxes[i] = new QCheckBox(phaseNames[i]);
        m_phaseCheckBoxes[i]->setChecked(true);

        controlBarLayout->addWidget(m_phaseCheckBoxes[i]);
    }

    mainLayout->addStretch(); // 임시 스트레치
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

    // QButtonGroup으로 상호 배타적 동작 설정
    m_buttonGroup = new QButtonGroup(this);
    m_buttonGroup->setExclusive(true); // 하나만 선택 가능
    m_buttonGroup->addButton(m_voltageButton, 0); // ID 0
    m_buttonGroup->addButton(m_currentButton, 1); // ID 1

    // 초기 선택
    m_voltageButton->setChecked(true); // Voltage가 기본 선택

    // 버튼을 topLayout에 추가
    topLayout->addWidget(m_voltageButton);
    topLayout->addWidget(m_currentButton);

    connect(m_buttonGroup, &QButtonGroup::idClicked, this, &AnalysisHarmonicPage::onDisplayTypeChanged);

    mainLayout->addLayout(topLayout);
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
