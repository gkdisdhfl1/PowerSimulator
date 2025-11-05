#include "a3700n_window.h"
#include "base_graph_window.h"
#include "custom_chart_view.h"
#include "phasor_view.h"
#include "simulation_engine.h"
#include <QHBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QTabWidget>
#include <QCheckBox>
#include <QLineSeries>
#include <QValueAxis>

// ==========================
// 단일 행 위젯
// ==========================
class DataRowWidget : public QWidget
{
    Q_OBJECT
public:
    DataRowWidget(const QString& name, const QString& unit, bool hasLine = true, QWidget* parent = nullptr)
        : QWidget(parent)
    {
        auto mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0); // 여백 없음
        mainLayout->setSpacing(4);// 라벨과 선 사이 간격

        // 1. 이름 - 값 - 단위를 담을 box
        auto rowLayout = new QHBoxLayout();

        QLabel* nameLabel = new QLabel(name, this);
        nameLabel->setMinimumWidth(60); // 이름 영역 너비 고정

        m_valueLabel = new QLabel("0.000", this);
        m_valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        QLabel* unitLabel = new QLabel(unit, this);
        unitLabel->setMinimumWidth(20); // 단위 영역 너비 고정

        nameLabel->setObjectName("nameLabel");
        m_valueLabel->setObjectName("valueLabel");
        unitLabel->setObjectName("unitLabel");

        rowLayout->addWidget(nameLabel);
        rowLayout->addWidget(m_valueLabel, 1); // 값 라벨이 남은 공간 모두 차지
        rowLayout->addWidget(unitLabel);

        mainLayout->addLayout(rowLayout);

        // 2. 하단 라인
        if(hasLine) {
            QFrame* hLine = new QFrame(this);
            hLine->setFrameShape(QFrame::HLine);
            hLine->setFixedHeight(1);
            hLine->setObjectName("hLine");

            mainLayout->addWidget(hLine);
        }

    }

public slots:
    void setValue(double value) {
        m_valueLabel->setText(QString::number(value, 'f', 3));
    }

private:
    QLabel* m_valueLabel;
};

// ================================
// DataPage
// ================================
class DataPage : public QWidget
{
    Q_OBJECT
public:
    using Extractor = std::function<double(const OneSecondSummaryData&)>;

    DataPage(const QString& title,
             const QStringList& rowLabels,
             const QString& unit,
             const std::vector<Extractor>& extractors, QWidget* parent = nullptr)
        : QWidget(parent), m_extractors(extractors)
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

public slots:
    void updateDataFromSummary(const OneSecondSummaryData& data)
    {
        for(size_t i = 0; i < m_extractors.size(); ++i) {
            if(i < m_rowWidgets.size()) {
                double value = m_extractors[i](data);
                m_rowWidgets[i]->setValue(value); // 각 행 위젯의 슬롯 호출
            }
        }
    }

private:
    std::vector<DataRowWidget*> m_rowWidgets;
    std::vector<Extractor> m_extractors;
};

// ================================
// Phasor 전용 위젯
// ================================
class AnalysisPhasorPage : public QWidget
{
    Q_OBJECT
public:
    AnalysisPhasorPage(QWidget* parent = nullptr) : QWidget(parent)
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
        voltageCheck->setObjectName("check");
        currentCheck->setObjectName("check");
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
        auto tableLayout = new QGridLayout();
        tableLayout->setContentsMargins(0, 0, 0, 0);
        tableLayout->setSpacing(0);

        // 전압 섹션
        m_voltageTableContainer = new QWidget(this);
        auto voltageSectionLayout = new QVBoxLayout(m_voltageTableContainer);
        voltageSectionLayout->setContentsMargins(0, 0, 0, 0);
        voltageSectionLayout->setSpacing(0);

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

        contentLayout->addLayout(tableLayout, 1);
        mainLayout->addLayout(contentLayout, 1);

        // 라벨 배경색 설정
        const auto& voltageColors = m_phasorView->getVoltageColors();
        const auto& currentColors = m_phasorView->getCurrentColors();

        for(int i{0}; i < 3; ++i) {
            QString voltageStyle = QString("background-color: %1; color: white;").arg(voltageColors[i].name());
            m_voltageNameLabels[i]->setStyleSheet(voltageStyle);

            QString currentStyle = QString("background-color: %1; color: white;").arg(currentColors[i].name());
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

public slots:
    void updateSummaryData(const OneSecondSummaryData& data)
    {
        // PhasorView 업데이트
        m_phasorView->updateData(data.fundamentalVoltage, data.fundamentalCurrent, data.lastCycleVoltageHarmonics, data.lastCycleCurrentHarmonics);

        // Voltage
        for(int i{0}; i < 3; ++i) {
            m_voltageTable[i * 2 + 0]->setText(QString::number(data.fundamentalVoltage[i].rms, 'f', 3));
            m_voltageTable[i * 2 + 1]->setText(QString::number(utils::radiansToDegrees(data.fundamentalVoltage[i].phase), 'f', 1) + "°");
        }

        // Current
        for(int i{0}; i < 3; ++i) {
            m_currentTable[i * 2 + 0]->setText(QString::number(data.fundamentalCurrent[i].rms, 'f', 3));
            m_currentTable[i * 2 + 1]->setText(QString::number(utils::radiansToDegrees(data.fundamentalCurrent[i].phase), 'f', 1) + "°");
        }
    }

private:
    std::pair<std::array<QLabel*, 3>, std::array<QLabel*, 6>> createPhasorTable(QVBoxLayout* layout, const QString& title, const QStringList& labels, const QString& unit) {
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

            // rowLayout->addStretch(1);

            QLabel* valueLabel = new QLabel("0.000", this);
            // valueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            valueLabel->setFixedWidth(60);
            QLabel* unitLabel = new QLabel(unit, this);
            QLabel* phaseLabel = new QLabel("0.0° ", this);
            phaseLabel->setFixedWidth(50);


            valueLabel->setObjectName("phasorValueLabel");
            unitLabel->setObjectName("phasorUnitLabel");
            phaseLabel->setObjectName("phasorValueLabel");

            valueLabels[i * 2 + 0] = valueLabel; // 값
            valueLabels[i * 2 + 1] = phaseLabel; // 위상

            valueLabels[i * 2 + 0]->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            valueLabels[i * 2 + 1]->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

            rowLayout->addWidget(valueLabels[i * 2 + 0], 1);
            rowLayout->addWidget(unitLabel);
            rowLayout->addWidget(valueLabels[i * 2 + 1], 1);

            layout->addLayout(rowLayout);
        }
        return {nameLabels, valueLabels};
    }

    PhasorView* m_phasorView;
    QWidget* m_voltageTableContainer;
    QWidget* m_currentTableContainer;
    std::array<QLabel*, 3> m_voltageNameLabels;
    std::array<QLabel*, 3> m_currentNameLabels;
    std::array<QLabel* , 6> m_voltageTable;
    std::array<QLabel*, 6> m_currentTable;
};

// ================================
// Waveform 전용 위젯
// ================================
class AnalysisWaveformPage : public BaseGraphWindow
{
    Q_OBJECT
public:
    AnalysisWaveformPage(SimulationEngine* engine, QWidget* parent = nullptr)
        : BaseGraphWindow(engine, parent)
    {
        m_chartView->setInteractive(false); // 스크롤, 줌 금지
        m_chart->legend()->hide(); // 범례 숨기기
        toggleAutoScroll(false); // 자동 스크롤 끄기

        m_axisY = new QValueAxis(this);
        m_axisY->setTitleText("V/A");
        m_chart->addAxis(m_axisY, Qt::AlignLeft);

        // A상 전압/전류 시리즈
        m_voltageSeries = new QLineSeries(this);
        m_currentSeries = new QLineSeries(this);
        m_voltageSeries->setColor(QColor("blue"));
        m_currentSeries->setColor(QColor("red"));

        m_chart->addSeries(m_voltageSeries);
        m_chart->addSeries(m_currentSeries);
        m_voltageSeries->attachAxis(m_axisX);
        m_voltageSeries->attachAxis(m_axisY);
        m_currentSeries->attachAxis(m_axisX);
        m_currentSeries->attachAxis(m_axisY);
    }

    void setupSeries() override {}

public slots:
    void updateWaveformData(const std::deque<DataPoint>& data)
    {
        if (data.empty()) return;

        // 1. 2 사이클에 해당하는 샘플 수 계산
        double freq = m_engine->m_frequency.value();
        if (freq < 0.1) freq = 0.1;

        double samplesPerCycle = m_engine->m_samplesPerCycle.value();
        int samplesToTake = static_cast<int>(samplesPerCycle * 2.0); // 2 사이클

        if (samplesToTake < 2) samplesToTake = 2;
        if (samplesToTake > data.size()) samplesToTake = data.size();

        // 2. 데이터의 마지막 N개(2 사이클 분량)를 가져옴
        QList<QPointF> vPoints, iPoints;
        vPoints.reserve(samplesToTake);
        iPoints.reserve(samplesToTake);

        double minY = std::numeric_limits<double>::max();
        double maxY = std::numeric_limits<double>::lowest();

        for (auto it = data.end() - samplesToTake; it != data.end(); ++it) {
            double timeSec = std::chrono::duration<double>(it->timestamp).count();
            double v = it->voltage.a;
            double i = it->current.a;

            vPoints.append(QPointF(timeSec, v));
            iPoints.append(QPointF(timeSec, i));

            minY = std::min({minY, v, i});
            maxY = std::max({maxY, v, i});
        }

        // 3. 시리즈 및 축 업데이트
        m_voltageSeries->replace(vPoints);
        m_currentSeries->replace(iPoints);

        if (!vPoints.isEmpty()) {
            m_axisX->setRange(vPoints.first().x(), vPoints.last().x());
            double padding = (maxY - minY) * 0.1 + 1.0;
            m_axisY->setRange(minY - padding, maxY + padding);
        }
    }

private:
    QLineSeries* m_voltageSeries;
    QLineSeries* m_currentSeries;
    QValueAxis* m_axisY;
};


// ==================================================

A3700N_Window::A3700N_Window(SimulationEngine* engine, QWidget *parent)
    : QWidget{parent}
    , m_engine(engine)
{
    setupUi();
    setFixedSize(500, 250);
}

void A3700N_Window::setupUi()
{
    // 메인 탭 생성
    m_mainTabs = new QTabWidget(this);
    m_mainTabs->setTabPosition(QTabWidget::North);

    // 각 탭 생성
    m_mainTabs->addTab(createTabPage("Voltage"), "VOLTAGE");
    m_mainTabs->addTab(createTabPage("Current"), "CURRENT");
    m_mainTabs->addTab(createTabPage("POWER"), "POWER");
    m_mainTabs->addTab(createTabPage("ANALYSIS"), "ANALYSIS");

    m_mainTabs->setObjectName("mainTabs");
    m_mainTabs->tabBar()->setObjectName("mainTabBar");

    auto layout  = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_mainTabs);
    setLayout(layout);

}

QWidget* A3700N_Window::createTabPage(const QString& type)
{
    // 왼쪽 서브메뉴 생성
    auto submenu = new QListWidget();
    submenu->setObjectName("submenuList");
    submenu->setMaximumWidth(120);
    submenu->setMinimumWidth(120);

    auto contentsStack = new QStackedWidget(this);
    contentsStack->setObjectName("contentsStack");

    QPalette pal = contentsStack->palette();

    // contentsStack 배경색 설정
    pal.setColor(QPalette::Window, Qt::white);
    contentsStack->setAutoFillBackground(true);
    contentsStack->setPalette(pal);

    // 페이지 생성
    if(type == "Voltage") {
        createVoltagePage(submenu, contentsStack);
    } else if(type == "Current") {
        createCurrentPage(submenu, contentsStack);
    } else if(type == "POWER") {
        createPowerPage(submenu, contentsStack);
    } else if(type == "ANALYSIS") {
        createAnalysisPage(submenu, contentsStack);
    }

    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(submenu);
    mainLayout->addWidget(contentsStack, 1);

    connect(submenu, &QListWidget::currentRowChanged, contentsStack, &QStackedWidget::setCurrentIndex);
    submenu->setCurrentRow(0);

    auto container = new QWidget();
    container->setLayout(mainLayout)    ;
    return container;
}

void A3700N_Window::createAndAddPage(
    QListWidget* submenu,
    QStackedWidget* stack,
    const QString& submenuName,
    const QString& title,
    const QStringList& rowLabels,
    const QString& unit,
    const std::vector<std::function<double(const OneSecondSummaryData&)>>& extractors)
{

    DataPage* page = new DataPage(title, rowLabels, unit, extractors);

    connect(this, &A3700N_Window::summaryDataUpdated, page, &DataPage::updateDataFromSummary);
    stack->addWidget(page);
    submenu->addItem(submenuName); // 메뉴 이름 설정
}

void A3700N_Window::updateSummaryData(const OneSecondSummaryData& data)
{
    emit summaryDataUpdated(data);
}
void A3700N_Window::updateWaveformData(const std::deque<DataPoint>& data)
{
    emit waveformDataUpdated(data);
}

void A3700N_Window::createVoltagePage(QListWidget* submenu, QStackedWidget* contentsStack)
{
    createAndAddPage(submenu, contentsStack, "RMS", "RMS Voltage", {"A", "B", "C", "Average"}, "V",
                     {
                         [](const auto& d) { return d.totalVoltageRms.a; },
                         [](const auto& d) { return d.totalVoltageRms.b; },
                         [](const auto& d) { return d.totalVoltageRms.c; },
                         [](const auto& d) { return (d.totalVoltageRms.a + d.totalVoltageRms.b + d.totalVoltageRms.c) / 3.0; }
                     });

    createAndAddPage(submenu, contentsStack, "Fundamental", "Fund. Volt.", {"A", "B", "C", "Average"}, "V",
                     {
                         [](const auto& d) { return d.fundamentalVoltage[0].rms; },
                         [](const auto& d) { return d.fundamentalVoltage[1].rms; },
                         [](const auto& d) { return d.fundamentalVoltage[2].rms; },
                         [](const auto& d) { return (d.fundamentalVoltage[0].rms + d.fundamentalVoltage[1].rms + d.fundamentalVoltage[2].rms) / 3.0; }
                     });

    createAndAddPage(submenu, contentsStack, "THD %", "Total Harmonic Distortion", {"A", "B", "C"}, "%",
                     {
                         [](const auto& d) { return d.voltageThd.a; },
                         [](const auto& d) { return d.voltageThd.b; },
                         [](const auto& d) { return d.voltageThd.c; }
                     });

    createAndAddPage(submenu, contentsStack, "Frequency", "Frequency", {"Frequency"}, "Hz",
                     {
                         [](const auto& d) { return d.frequency; }
                     });

    createAndAddPage(submenu, contentsStack, "Residual", "Residual Voltage", {"RMS", "Fund."}, "V",
                     {
                         [](const auto& d) { return d.residualVoltageRms; },
                         [](const auto& d) { return d.residualVoltageFundamental; }
                     });
}

void A3700N_Window::createCurrentPage(QListWidget* submenu, QStackedWidget* contentsStack)
{
    createAndAddPage(submenu, contentsStack, "RMS", "RMS Current", {"A", "B", "C", "Average"}, "A",
                     {
                         [](const auto& d) { return d.totalCurrentRms.a; },
                         [](const auto& d) { return d.totalCurrentRms.b; },
                         [](const auto& d) { return d.totalCurrentRms.c; },
                         [](const auto& d) { return (d.totalCurrentRms.a + d.totalCurrentRms.b + d.totalCurrentRms.c) / 3.0; }
                     });

    createAndAddPage(submenu, contentsStack, "Fundamental", "Fundamental Current.", {"A", "B", "C", "Average"}, "A",
                     {
                         [](const auto& d) { return d.fundamentalCurrent[0].rms; },
                         [](const auto& d) { return d.fundamentalCurrent[1].rms; },
                         [](const auto& d) { return d.fundamentalCurrent[2].rms; },
                         [](const auto& d) { return (d.fundamentalCurrent[0].rms + d.fundamentalCurrent[1].rms + d.fundamentalCurrent[2].rms) / 3.0; }
                     });

    createAndAddPage(submenu, contentsStack, "THD %", "Total Harmonic Distortion", {"A", "B", "C"}, "%",
                     {
                         [](const auto& d) { return d.currentThd.a; },
                         [](const auto& d) { return d.currentThd.b; },
                         [](const auto& d) { return d.currentThd.c; }
                     });

    createAndAddPage(submenu, contentsStack, "Residual", "Residual Current", {"RMS", "Fund."}, "A",
                     {
                         [](const auto& d) { return d.residualCurrentRms; },
                         [](const auto& d) { return d.residualCurrentFundamental; }
                     });
}

void A3700N_Window::createPowerPage(QListWidget* submenu, QStackedWidget* stack)
{
    createAndAddPage(submenu, stack, "Active(P)", "Active Power", {"A", "B", "C", "Total"}, "kW",
                     {
                         [](const OneSecondSummaryData& d) { return d.activePower.a / 1e3; },
                         [](const OneSecondSummaryData& d) { return d.activePower.b / 1e3; },
                         [](const OneSecondSummaryData& d) { return d.activePower.c / 1e3; },
                         [](const OneSecondSummaryData& d) { return d.totalActivePower / 1e3; }
                     });

    createAndAddPage(submenu, stack, "Reactive(Q)", "Reactive Power", {"A", "B", "C", "Total"}, "kVAR",
                     {
                         [](const OneSecondSummaryData& d) { return d.reactivePower.a / 1e3; },
                         [](const OneSecondSummaryData& d) { return d.reactivePower.b / 1e3; },
                         [](const OneSecondSummaryData& d) { return d.reactivePower.c / 1e3; },
                         [](const OneSecondSummaryData& d) { return d.totalReactivePower / 1e3; }
                     });

    createAndAddPage(submenu, stack, "Apparent(S)", "Apparent Power", {"A", "B", "C", "Total"}, "kVA",
                     {
                         [](const OneSecondSummaryData& d) { return d.apparentPower.a / 1e3; },
                         [](const OneSecondSummaryData& d) { return d.apparentPower.b / 1e3; },
                         [](const OneSecondSummaryData& d) { return d.apparentPower.c / 1e3; },
                         [](const OneSecondSummaryData& d) { return d.totalApparentPower / 1e3; }
                     });

    createAndAddPage(submenu, stack, "PF", "Power Factor", {"A", "B", "C", "Total"}, "",
                     {
                         [](const OneSecondSummaryData& d) { return d.powerFactor.a; },
                         [](const OneSecondSummaryData& d) { return d.powerFactor.b; },
                         [](const OneSecondSummaryData& d) { return d.powerFactor.c; },
                         [](const OneSecondSummaryData& d) { return d.totalPowerFactor; }
                     });

    createAndAddPage(submenu, stack, "Energy", "Energy", {"Net"}, "kWh",
                     {
                         [](const OneSecondSummaryData& d) { return d.totalEnergyWh / 1e3; }
                     });
}

void A3700N_Window::createAnalysisPage(QListWidget* submenu, QStackedWidget* stack)
{
    // Phasor 페이지
    AnalysisPhasorPage* phasorPage = new AnalysisPhasorPage(this);

    connect(this, &A3700N_Window::summaryDataUpdated, phasorPage, &AnalysisPhasorPage::updateSummaryData);
    stack->addWidget(phasorPage);
    submenu->addItem("Phasor");

    // Waveform 페이지
    AnalysisWaveformPage* waveformPage = new AnalysisWaveformPage(m_engine);

    connect(this, &A3700N_Window::waveformDataUpdated, waveformPage, &AnalysisWaveformPage::updateWaveformData);
    stack->addWidget(waveformPage);
    submenu->addItem("Waveform");

    // 나머지
    createAndAddPage(submenu, stack, "Volt. Symm.", "Symmetrical Component", {"Positive-\nSequence", "Negative-\nSequence", "Zero-\nSequence"}, "V",
                     {
                         [](const OneSecondSummaryData& d) { return d.voltageSymmetricalComponents.positive.magnitude; },
                         [](const OneSecondSummaryData& d) { return d.voltageSymmetricalComponents.negative.magnitude; },
                         [](const OneSecondSummaryData& d) { return d.voltageSymmetricalComponents.zero.magnitude; },
                     });

    createAndAddPage(submenu, stack, "Volt. Unbal. %", "Voltage Unbalance", {"NEMA", "Negative-\nSequence", "Zero-\nSequence"}, "%",
                     {
                         [](const OneSecondSummaryData& d) { return d.nemaVoltageUnbalance; },
                         [](const OneSecondSummaryData& d) { return d.voltageU2Unbalance; },
                         [](const OneSecondSummaryData& d) { return d.voltageU0Unbalance; }
                     });
    createAndAddPage(submenu, stack, "Curr. Symm.", "Curr. Symm. Component", {"Positive-\nSequence", "Negative-\nSequence", "Zero-\nSequence"}, "A",
                     {
                      [](const OneSecondSummaryData& d) { return d.currentSymmetricalComponents.positive.magnitude; },
                      [](const OneSecondSummaryData& d) { return d.currentSymmetricalComponents.negative.magnitude; },
                      [](const OneSecondSummaryData& d) { return d.currentSymmetricalComponents.zero.magnitude; },
                      });

    createAndAddPage(submenu, stack, "Curr. Unbal. %", "Current Unbalance", {"NEMA", "Negative-\nSequence", "Zero-\nSequence"}, "%",
                     {
                         [](const OneSecondSummaryData& d) { return d.nemaCurrentUnbalance; },
                         [](const OneSecondSummaryData& d) { return d.currentU2Unbalance; },
                         [](const OneSecondSummaryData& d) { return d.currentU0Unbalance; }
                     });
}

#include "a3700n_window.moc"

