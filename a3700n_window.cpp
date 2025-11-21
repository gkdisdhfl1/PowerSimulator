#include "a3700n_window.h"
#include "data_page.h"
#include "analysis_waveform_page.h"
#include "analysis_phasor_page.h"
#include "analysis_harmonic_page.h"
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
    // RMS 페이지
    std::vector<DataSource> rmsSources;
    rmsSources.push_back({
        "L-L",
        {"AB", "BC", "CA", "Average"},
        {
            [](const OneSecondSummaryData& d) { return d.totalVoltageRms_ll.ab; },
            [](const OneSecondSummaryData& d) { return d.totalVoltageRms_ll.bc; },
            [](const OneSecondSummaryData& d) { return d.totalVoltageRms_ll.ca; },
            [](const OneSecondSummaryData& d) { return (d.totalVoltageRms_ll.ab + d.totalVoltageRms_ll.bc+ d.totalVoltageRms_ll.ca) / 3.0; }
        },
        // Max Extractor
        {
            [](const DemandData& d) { return d.totalVoltageRms_ll.ab.max; },
            [](const DemandData& d) { return d.totalVoltageRms_ll.bc.max; },
            [](const DemandData& d) { return d.totalVoltageRms_ll.ca.max; },
            [](const DemandData& d) { return d.averageTotalVoltageRms_ll.max; }
        },
        // Min Extractor
        {
            [](const DemandData& d) { return d.totalVoltageRms_ll.ab.min; },
            [](const DemandData& d) { return d.totalVoltageRms_ll.bc.min; },
            [](const DemandData& d) { return d.totalVoltageRms_ll.ca.min; },
            [](const DemandData& d) { return d.averageTotalVoltageRms_ll.min; }
        }
    });

    rmsSources.push_back({
        "L-N",
        {"A", "B", "C", "Average"},
        {
            [](const OneSecondSummaryData& d) { return d.totalVoltageRms.a; },
            [](const OneSecondSummaryData& d) { return d.totalVoltageRms.b; },
            [](const OneSecondSummaryData& d) { return d.totalVoltageRms.c; },
            [](const OneSecondSummaryData& d) { return (d.totalVoltageRms.a + d.totalVoltageRms.b + d.totalVoltageRms.c) / 3.0; }
        },
        // Max Extractor
        {
            [](const DemandData& d) { return d.totalVoltageRms.a.max; },
            [](const DemandData& d) { return d.totalVoltageRms.b.max; },
            [](const DemandData& d) { return d.totalVoltageRms.c.max; },
            [](const DemandData& d) { return d.averageTotalVoltageRms.max; }
        },
        // Min Extractor
        {
            [](const DemandData& d) { return d.totalVoltageRms.a.min; },
            [](const DemandData& d) { return d.totalVoltageRms.b.min; },
            [](const DemandData& d) { return d.totalVoltageRms.c.min; },
            [](const DemandData& d) { return d.averageTotalVoltageRms.min; }
        }
    });

    createAndAddPage(submenu, contentsStack, "RMS", "RMS Voltage", "V", rmsSources);

    // Fundamental 페이지
    std::vector<DataSource> fundamentalSources;
    fundamentalSources.push_back({
        "L-L",
        {"AB", "BC", "CA", "Average"},
        {
            [](const OneSecondSummaryData& d) { return d.fundamentalVoltage_ll[0].rms; },
            [](const OneSecondSummaryData& d) { return d.fundamentalVoltage_ll[1].rms; },
            [](const OneSecondSummaryData& d) { return d.fundamentalVoltage_ll[2].rms; },
            [](const OneSecondSummaryData& d) { return (d.fundamentalVoltage_ll[0].rms + d.fundamentalVoltage_ll[1].rms + d.fundamentalVoltage_ll[2].rms) / 3.0; }
        },
        // Max Extractor
        {
            [](const DemandData& d) { return d.fundamentalVoltageRMS_ll.ab.max; },
            [](const DemandData& d) { return d.fundamentalVoltageRMS_ll.bc.max; },
            [](const DemandData& d) { return d.fundamentalVoltageRMS_ll.ca.max; },
            [](const DemandData& d) { return d.averageFundamentalVoltageRms_ll.max; }
        },
        // Min Extractor
        {
            [](const DemandData& d) { return d.fundamentalVoltageRMS_ll.ab.min; },
            [](const DemandData& d) { return d.fundamentalVoltageRMS_ll.bc.min; },
            [](const DemandData& d) { return d.fundamentalVoltageRMS_ll.ca.min; },
            [](const DemandData& d) { return d.averageFundamentalVoltageRms_ll.min; }
        }
    });
    fundamentalSources.push_back({
        "L-N",
        {"A", "B", "C", "Average"},
        {
            [](const OneSecondSummaryData& d) { return d.fundamentalVoltage[0].rms; },
            [](const OneSecondSummaryData& d) { return d.fundamentalVoltage[1].rms; },
            [](const OneSecondSummaryData& d) { return d.fundamentalVoltage[2].rms; },
            [](const OneSecondSummaryData& d) { return (d.fundamentalVoltage[0].rms + d.fundamentalVoltage[1].rms + d.fundamentalVoltage[2].rms) / 3.0; }
        },
        // Max Extractor
        {
            [](const DemandData& d) { return d.fundamentalVoltageRMS.a.max; },
            [](const DemandData& d) { return d.fundamentalVoltageRMS.b.max; },
            [](const DemandData& d) { return d.fundamentalVoltageRMS.c.max; },
            [](const DemandData& d) { return d.averageFundamentalVoltageRms.max; }
        },
        // Min Extractor
        {
            [](const DemandData& d) { return d.fundamentalVoltageRMS.a.min; },
            [](const DemandData& d) { return d.fundamentalVoltageRMS.b.min; },
            [](const DemandData& d) { return d.fundamentalVoltageRMS.c.min; },
            [](const DemandData& d) { return d.averageFundamentalVoltageRms.min; }
        }
    });

    createAndAddPage(submenu, contentsStack, "Fundamental", "Fund. Volt.", "V", fundamentalSources);

    // THD 페이지
    std::vector<DataSource> thdSources;
    thdSources.push_back({
        "L-L",
        {"AB", "BC", "CA"},
        {
             [](const OneSecondSummaryData& d) { return d.voltageThd_ll.ab; },
             [](const OneSecondSummaryData& d) { return d.voltageThd_ll.bc; },
             [](const OneSecondSummaryData& d) { return d.voltageThd_ll.ca; },
         },
        // Max Extractor
        {
            [](const DemandData& d) { return d.voltageThd_ll.ab; },
            [](const DemandData& d) { return d.voltageThd_ll.bc; },
            [](const DemandData& d) { return d.voltageThd_ll.ca; }
        }
    });
    thdSources.push_back({
        "L-N",
        {"A", "B", "C"},
        {
            [](const OneSecondSummaryData& d) { return d.voltageThd.a; },
            [](const OneSecondSummaryData& d) { return d.voltageThd.b; },
            [](const OneSecondSummaryData& d) { return d.voltageThd.c; },
         },
        // Max Extractor
        {
            [](const DemandData& d) { return d.voltageThd.a; },
            [](const DemandData& d) { return d.voltageThd.b; },
            [](const DemandData& d) { return d.voltageThd.c; }
        }
    });
    createAndAddPage(submenu, contentsStack, "THD %", "Total Harmonic Distortion", "%", thdSources);


    createAndAddPage(submenu, contentsStack, "Frequency", "Frequency", "Hz",
                     {{ "",
                         {"Frequency"},
                         {
                             [](const auto& d) { return d.frequency; }
                         },
                         // Max Extractor
                         {
                          [](const DemandData& d) { return d.frequency.max; },
                          },
                         // Min Extractor
                         {
                          [](const DemandData& d) { return d.frequency.min; },
                          }
                     }});

    createAndAddPage(submenu, contentsStack, "Residual", "Residual Voltage", "V",
                    {{ "",
                        {"RMS", "Fund."},
                        {
                            [](const auto& d) { return d.residualVoltageRms; },
                            [](const auto& d) { return d.residualVoltageFundamental; }
                        },
                        {
                            [](const DemandData& d) { return d.voltageResidualRms.max; },
                            [](const DemandData& d) { return d.voltageResidualFundamental.max; },
                        },
                        {
                            [](const DemandData& d) { return d.voltageResidualRms.min; },
                            [](const DemandData& d) { return d.voltageResidualFundamental.min; },
                        },
                    }});
}

void A3700N_Window::createCurrentPage(QListWidget* submenu, QStackedWidget* contentsStack)
{
    createAndAddPage(submenu, contentsStack, "RMS", "RMS Current", "A",
                     {{ "",
                         {"A", "B", "C", "Average"},
                         {
                             [](const auto& d) { return d.totalCurrentRms.a; },
                             [](const auto& d) { return d.totalCurrentRms.b; },
                             [](const auto& d) { return d.totalCurrentRms.c; },
                             [](const auto& d) { return (d.totalCurrentRms.a + d.totalCurrentRms.b + d.totalCurrentRms.c) / 3.0; }
                         },
                         // Max Extractor
                         {
                             [](const DemandData& d) { return d.totalCurrentRms.a.max; },
                             [](const DemandData& d) { return d.totalCurrentRms.b.max; },
                             [](const DemandData& d) { return d.totalCurrentRms.c.max; },
                             [](const DemandData& d) { return d.averageTotalCurrentRms.max; }
                         },
                         // Min Extractor
                         {
                             [](const DemandData& d) { return d.totalCurrentRms.a.min; },
                             [](const DemandData& d) { return d.totalCurrentRms.b.min; },
                             [](const DemandData& d) { return d.totalCurrentRms.c.min; },
                             [](const DemandData& d) { return d.averageTotalCurrentRms.min; }
                         }
                     }});

    createAndAddPage(submenu, contentsStack, "Fundamental", "Fundamental Current.", "A",
                     {{ "",
                         {"A", "B", "C", "Average"},
                         {
                             [](const auto& d) { return d.fundamentalCurrent[0].rms; },
                             [](const auto& d) { return d.fundamentalCurrent[1].rms; },
                             [](const auto& d) { return d.fundamentalCurrent[2].rms; },
                             [](const auto& d) { return (d.fundamentalCurrent[0].rms + d.fundamentalCurrent[1].rms + d.fundamentalCurrent[2].rms) / 3.0; }
                         },
                         // Max Extractor
                         {
                             [](const DemandData& d) { return d.fundamentalCurrentRMS.a.max; },
                             [](const DemandData& d) { return d.fundamentalCurrentRMS.b.max; },
                             [](const DemandData& d) { return d.fundamentalCurrentRMS.c.max; },
                             [](const DemandData& d) { return d.averageFundamentalCurrentRms.max; }
                         },
                         // Min Extractor
                         {
                             [](const DemandData& d) { return d.fundamentalCurrentRMS.a.min; },
                             [](const DemandData& d) { return d.fundamentalCurrentRMS.b.min; },
                             [](const DemandData& d) { return d.fundamentalCurrentRMS.c.min; },
                             [](const DemandData& d) { return d.averageFundamentalCurrentRms.min; }
                         }
                     }});
    
    createAndAddPage(submenu, contentsStack, "THD %", "Total Harmonic Distortion", "%",
                     {{ "",
                         {"A", "B", "C"},
                         {
                             [](const auto& d) { return d.currentThd.a; },
                             [](const auto& d) { return d.currentThd.b; },
                             [](const auto& d) { return d.currentThd.c; }
                         },
                         // Max Extractor
                         {
                             [](const DemandData& d) { return d.currentThd.a; },
                             [](const DemandData& d) { return d.currentThd.b; },
                             [](const DemandData& d) { return d.currentThd.c; }
                         }
                     }});

    createAndAddPage(submenu, contentsStack, "Residual", "Residual Current", "A",
                     {{ "",
                       {"RMS", "Fund."},
                       {
                           [](const auto& d) { return d.residualCurrentRms; },
                           [](const auto& d) { return d.residualCurrentFundamental; }
                       },
                       {
                        [](const DemandData& d) { return d.currentResidualRms.max; },
                        [](const DemandData& d) { return d.currentResidualFundamental.max; },
                        },
                       {
                           [](const DemandData& d) { return d.currentResidualRms.min; },
                           [](const DemandData& d) { return d.currentResidualFundamental.min; },
                           },
                       }});
}

void A3700N_Window::createPowerPage(QListWidget* submenu, QStackedWidget* stack)
{
    createAndAddPage(submenu, stack, "Active(P)", "Active Power", "kW",
                     {{ "",
                         {"A", "B", "C", "Total"},
                         {
                             [](const OneSecondSummaryData& d) { return d.activePower.a / 1e3; },
                             [](const OneSecondSummaryData& d) { return d.activePower.b / 1e3; },
                             [](const OneSecondSummaryData& d) { return d.activePower.c / 1e3; },
                             [](const OneSecondSummaryData& d) { return d.totalActivePower / 1e3; }
                         },
                         // Max Extractor
                         {
                             [](const DemandData& d) { return d.activePower.a.max; },
                             [](const DemandData& d) { return d.activePower.b.max; },
                             [](const DemandData& d) { return d.activePower.c.max; },
                             [](const DemandData& d) { return d.totalActivePower.max; }
                         },
                         // Min Extractor
                         {
                             [](const DemandData& d) { return d.activePower.a.min; },
                             [](const DemandData& d) { return d.activePower.b.min; },
                             [](const DemandData& d) { return d.activePower.c.min; },
                             [](const DemandData& d) { return d.totalActivePower.min; }
                         }
                     }});

    createAndAddPage(submenu, stack, "Reactive(Q)", "Reactive Power", "kVAR",
                     {{ "",
                         {"A", "B", "C", "Total"},
                         {
                             [](const OneSecondSummaryData& d) { return d.reactivePower.a / 1e3; },
                             [](const OneSecondSummaryData& d) { return d.reactivePower.b / 1e3; },
                             [](const OneSecondSummaryData& d) { return d.reactivePower.c / 1e3; },
                             [](const OneSecondSummaryData& d) { return d.totalReactivePower / 1e3; }
                         },
                         // Max Extractor
                         {
                             [](const DemandData& d) { return d.reactivePower.a.max; },
                             [](const DemandData& d) { return d.reactivePower.b.max; },
                             [](const DemandData& d) { return d.reactivePower.c.max; },
                             [](const DemandData& d) { return d.totalReactivePower.max; }
                         },
                         // Min Extractor
                         {
                             [](const DemandData& d) { return d.reactivePower.a.min; },
                             [](const DemandData& d) { return d.reactivePower.b.min; },
                             [](const DemandData& d) { return d.reactivePower.c.min; },
                             [](const DemandData& d) { return d.totalReactivePower.min; }
                         }
                     }});

    createAndAddPage(submenu, stack, "Apparent(S)", "Apparent Power", "kVA",
                     {{ "",
                         {"A", "B", "C", "Total"},
                         {
                             [](const OneSecondSummaryData& d) { return d.apparentPower.a / 1e3; },
                             [](const OneSecondSummaryData& d) { return d.apparentPower.b / 1e3; },
                             [](const OneSecondSummaryData& d) { return d.apparentPower.c / 1e3; },
                             [](const OneSecondSummaryData& d) { return d.totalApparentPower / 1e3; }
                         },
                         // Max Extractor
                         {
                             [](const DemandData& d) { return d.apparentPower.a.max; },
                             [](const DemandData& d) { return d.apparentPower.b.max; },
                             [](const DemandData& d) { return d.apparentPower.c.max; },
                             [](const DemandData& d) { return d.totalApparentPower.max; }
                         },
                         // Min Extractor
                         {
                             [](const DemandData& d) { return d.apparentPower.a.min; },
                             [](const DemandData& d) { return d.apparentPower.b.min; },
                             [](const DemandData& d) { return d.apparentPower.c.min; },
                             [](const DemandData& d) { return d.totalApparentPower.min; }
                         }
                     }});

    createAndAddPage(submenu, stack, "PF", "Power Factor", "",
                     {{ "",
                         {"A", "B", "C", "Total"},
                         {
                             [](const OneSecondSummaryData& d) { return d.powerFactor.a; },
                             [](const OneSecondSummaryData& d) { return d.powerFactor.b; },
                             [](const OneSecondSummaryData& d) { return d.powerFactor.c; },
                             [](const OneSecondSummaryData& d) { return d.totalPowerFactor; }
                         },
                         // Max Extractor
                         {
                             [](const DemandData& d) { return d.powerFactor.a.max; },
                             [](const DemandData& d) { return d.powerFactor.b.max; },
                             [](const DemandData& d) { return d.powerFactor.c.max; },
                             [](const DemandData& d) { return d.totalPowerFactor.max; }
                         },
                         // Min Extractor
                         {
                             [](const DemandData& d) { return d.powerFactor.a.min; },
                             [](const DemandData& d) { return d.powerFactor.b.min; },
                             [](const DemandData& d) { return d.powerFactor.c.min; },
                             [](const DemandData& d) { return d.totalPowerFactor.min; }
                         }
                     }});

    createAndAddPage(submenu, stack, "Energy", "Energy", "kWh",
                     {{ "",
                         {"A", "B", "C", "Total"},
                         {
                             [](const OneSecondSummaryData& d) { return d.totalEnergyWh / 1e3; }
                         }
                     }});
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

    // Volt. Symm 페이지
    std::vector<DataSource> voltSymSource;
    voltSymSource.push_back({
        "L-L",
        {"Positive\nSequence", "Negative-\nSequence"},
        {
            [](const OneSecondSummaryData& d) { return d.voltageSymmetricalComponents_ll.positive.magnitude; },
            [](const OneSecondSummaryData& d) { return d.voltageSymmetricalComponents_ll.negative.magnitude;}
        }
    });
    voltSymSource.push_back({
        "L-N",
        {"Positive-\nSequence", "Negative-\nSequence", "Zero-\nSequence"},
        {
            [](const OneSecondSummaryData& d) { return d.voltageSymmetricalComponents.positive.magnitude; },
            [](const OneSecondSummaryData& d) { return d.voltageSymmetricalComponents.negative.magnitude; },
            [](const OneSecondSummaryData& d) { return d.voltageSymmetricalComponents.zero.magnitude; }
        }
    });
    createAndAddPage(submenu, stack, "Volt. Symm.", "Symmetrical Component", "V", voltSymSource);

    createAndAddPage(submenu, stack, "Volt. Unbal. %", "Voltage Unbalance", "%",
                     {{ "",
                         {"NEMA", "NEMA", "Negative-\nSequence", "Zero-\nSequence"},
                         {
                             [](const OneSecondSummaryData& d) { return d.nemaVoltageUnbalance_ll; },
                             [](const OneSecondSummaryData& d) { return d.nemaVoltageUnbalance; },
                             [](const OneSecondSummaryData& d) { return d.voltageU2Unbalance; },
                             [](const OneSecondSummaryData& d) { return d.voltageU0Unbalance; }
                         }
                     }});

    createAndAddPage(submenu, stack, "Curr. Symm.", "Curr. Symm. Component", "A",
                     {{ "",
                         {"Positive-\nSequence", "Negative-\nSequence", "Zero-\nSequence"},
                         {
                          [](const OneSecondSummaryData& d) { return d.currentSymmetricalComponents.positive.magnitude; },
                          [](const OneSecondSummaryData& d) { return d.currentSymmetricalComponents.negative.magnitude; },
                          [](const OneSecondSummaryData& d) { return d.currentSymmetricalComponents.zero.magnitude; },
                          }
                     }});

    createAndAddPage(submenu, stack, "Curr. Unbal. %", "Current Unbalance", "%",
                     {{ "",
                         {"NEMA", "Negative-\nSequence", "Zero-\nSequence"},
                         {
                             [](const OneSecondSummaryData& d) { return d.nemaCurrentUnbalance; },
                             [](const OneSecondSummaryData& d) { return d.currentU2Unbalance; },
                             [](const OneSecondSummaryData& d) { return d.currentU0Unbalance; }
                         }
                     }});
}

