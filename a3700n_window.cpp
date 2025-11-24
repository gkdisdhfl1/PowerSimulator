#include "a3700n_window.h"
#include "data_page.h"
#include "analysis_waveform_page.h"
#include "analysis_phasor_page.h"
#include "analysis_harmonic_page.h"
#include "a3700n_datasource_factory.h"
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

A3700N_Window::A3700N_Window(QWidget *parent)
    : QWidget{parent}
{
    setupUi();
    setFixedSize(600, 325);
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
    container->setLayout(mainLayout);
    return container;
}

void A3700N_Window::createAndAddPage(
    QListWidget* submenu,
    QStackedWidget* stack,
    const QString& submenuName,
    const QString& title,
    const QString& unit,
    const std::vector<DataSource>& dataSources)
{

    DataPage* page = new DataPage(title, unit, dataSources);

    connect(this, &A3700N_Window::summaryDataUpdated, page, &DataPage::onDataUpdated);
    connect(this, &A3700N_Window::demandDataUpdated, page, &DataPage::onDemandDataUpdated);
    stack->addWidget(page);
    submenu->addItem(submenuName); // 메뉴 이름 설정
}

void A3700N_Window::updateSummaryData(const OneSecondSummaryData& data)
{
    emit summaryDataUpdated(data);
}

void A3700N_Window::updateDemandData(const DemandData& data)
{
    emit demandDataUpdated(data);
}

void A3700N_Window::createVoltagePage(QListWidget* submenu, QStackedWidget* contentsStack)
{
    const std::vector<PageConfig> pageConfigs = {
        {
            "RMS", "RMS Voltage", "V",
            {
                DataSourceFactory::createVoltageRmsLLSource(),
                DataSourceFactory::createVoltageRmsLNSource()
            }
        },
        {
            "Fundamental", "Fund. Volt.", "V",
            {
                DataSourceFactory::createVoltageFundamentalLLSource(),
                DataSourceFactory::createVoltageFundamentalLNSource()
            }
        },
        {
            "THD %", "Total Harmonic Distortion", "%",
            {
                DataSourceFactory::createVoltageThdLLSource(),
                DataSourceFactory::createVoltageThdLNSource()
            },
        },
        {
            "Frequency", "Frequency", "Hz",
            {
                DataSourceFactory::createVoltageFrequencySource()
            }
        },
        {
            "Residual", "Residual Voltage", "V",
            {
                DataSourceFactory::createVoltageResidualSource()
            }
        }
    };

    for(const auto& config : pageConfigs) {
        createAndAddPage(submenu, contentsStack, config.submenuName, config.title, config.unit, config.dataSources);
    }
}

void A3700N_Window::createCurrentPage(QListWidget* submenu, QStackedWidget* contentsStack)
{
    const std::vector<PageConfig> pageConfigs = {
        {
            "RMS", "RMS Current", "A",
            {
                DataSourceFactory::createCurrentRmsSource()
            }
        },
        {
            "Fundamental", "Fundamental Current.", "A",
            {
                DataSourceFactory::createCurrentFundamentalSource()
            }
        },
        {
            "THD %", "Total Harmonic Distortion", "%",
            {
                DataSourceFactory::createCurrentThdSource()
            }
        },
        {
            "Residual", "Residual Current", "A",
            {
                DataSourceFactory::createCurrentResidualSource()
            }
        }
    };

    for(const auto& config : pageConfigs) {
        createAndAddPage(submenu, contentsStack, config.submenuName, config.title, config.unit, config.dataSources);
    }
}

void A3700N_Window::createPowerPage(QListWidget* submenu, QStackedWidget* stack)
{
    const std::vector<PageConfig> pageConfigs = {
        {
            "Active(P)", "Active Power", "kW",
            {
                DataSourceFactory::createPowerActiveSource()
            }
        },
        {
            "Reactive(Q)", "Reactive Power", "kVAR",
            {
                DataSourceFactory::createPowerReactiveSource()
            }
        },
        {
            "Apparent(S)", "Apparent Power", "kVA",
            {
                DataSourceFactory::createPowerApparentSource()
            }
        },
        {
            "PF", "Power Factor", "",
            {
                DataSourceFactory::createPowerFactorSource()
            }
        },
        {
            "Energy", "Energy", "kWh",
            {
                DataSourceFactory::createPowerEnergySource()
            }
        }
    };

    for(const auto& config : pageConfigs) {
        createAndAddPage(submenu, stack, config.submenuName, config.title, config.unit, config.dataSources);
    }
}

void A3700N_Window::createAnalysisPage(QListWidget* submenu, QStackedWidget* stack)
{
    // Phasor 페이지
    AnalysisPhasorPage* phasorPage = new AnalysisPhasorPage(this);

    connect(this, &A3700N_Window::summaryDataUpdated, phasorPage, &AnalysisPhasorPage::updateSummaryData);
    stack->addWidget(phasorPage);
    submenu->addItem("Phasor");

    // Harmonics 페이지
    AnalysisHarmonicPage* harmonicsPage = new AnalysisHarmonicPage(this);

    connect(this, &A3700N_Window::summaryDataUpdated, harmonicsPage, &AnalysisHarmonicPage::onOneSecondDataUpdated);
    stack->addWidget(harmonicsPage);
    submenu->addItem("Harmonics");

    // Waveform 페이지
    AnalysisWaveformPage* waveformPage = new AnalysisWaveformPage(this);

    connect(this, &A3700N_Window::summaryDataUpdated, waveformPage, &AnalysisWaveformPage::onOneSecondDataUpdated);
    stack->addWidget(waveformPage);
    submenu->addItem("Waveform");

    // 나머지
    const std::vector<PageConfig> pageConfigs = {
        {
            "Volt. Symm.", "Symmetrical Component", "V",
            {
                DataSourceFactory::createVoltageSymmetricalLLSource(),
                DataSourceFactory::createVoltageSymmetricalLNSource()
            }
        },
        {
            "Volt. Unbal. %", "Voltage Unbalance", "%",
            {
                DataSourceFactory::createVoltageUnbalanceSource()
            }
        },
        {
            "Curr. Symm.", "Curr. Symm. Component", "A",
            {
                DataSourceFactory::createCurrentSymmetricalSource()
            }
        },
        {
            "Curr. Unbal. %", "Current Unbalance", "%",
            {
                DataSourceFactory::createCurrentUnbalanceSource()
            }
        }
    };

    for(const auto& config : pageConfigs) {
        createAndAddPage(submenu, stack, config.submenuName, config.title, config.unit, config.dataSources);
    }
}

