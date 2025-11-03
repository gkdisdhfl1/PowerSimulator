#include "a3700n_window.h"
#include <QHBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QTabWidget>

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


// ==================================================

A3700N_Window::A3700N_Window(QWidget *parent)
    : QWidget{parent}
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
    submenu->setMaximumWidth(150);

    auto contentsStack = new QStackedWidget(this);
    contentsStack->setObjectName("contentsStack");

    QPalette pal = contentsStack->palette();

    // contentsStack 배경색 설정
    pal.setColor(QPalette::Window, Qt::white);
    contentsStack->setAutoFillBackground(true);
    contentsStack->setPalette(pal);

    if(type == "Voltage") {
        createVoltagePage(submenu, contentsStack);
    } else if(type == "Current") {
        createCurrentPage(submenu, contentsStack);
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

    m_pages.push_back(page);
    stack->addWidget(page);
    submenu->addItem(submenuName); // 메뉴 이름 설정
}

void A3700N_Window::updateData(const OneSecondSummaryData& data)
{
    for(DataPage* page : m_pages) {
        page->updateDataFromSummary(data);
    }
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
#include "a3700n_window.moc"
