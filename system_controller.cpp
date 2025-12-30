#include "system_controller.h"
#include "main_window.h"
#include "settings_manager.h"
#include "settings_ui_controller.h"
#include "simulation_engine.h"
#include "control_panel.h"
#include "graph_window.h"
#include "analysis_graph_window.h"
#include "fundamental_analysis_graph_window.h"
#include "harmonic_analysis_graph_window.h"
#include "phasor_view.h"
#include "three_phase_dialog.h"
#include "pid_tuning_dialog.h"
#include "one_second_summary_window.h"
#include "additional_metrics_window.h"
#include "a3700n_window.h"
#include "demand_calculator.h"

#include <QApplication>
#include <QMessageBox>

SystemController::SystemController(QObject *parent)
    : QObject{parent}
{}

SystemController::~SystemController()
{
    // 스레드 종료 및 정리
    if(m_simulationThread.isRunning()) {
        m_simulationThread.quit();
        m_simulationThread.wait();
    }
}

void SystemController::initialize()
{
    // 데이터 베이스 설정
    QString dbPath = QApplication::applicationDirPath() + "/settings.db";
    m_settingsManager = std::make_unique<SettingsManager>(dbPath.toStdString());

    m_mainWindow = std::make_unique<MainWindow>();

    setupThread();

    m_settingsController = std::make_unique<SettingsUiController>(
        m_mainWindow->getControlPanel(), *m_settingsManager, m_mainWindow.get())    ;

    createConnections();

    m_mainWindow->show();
}

void SystemController::setupThread()
{
    m_engine = new SimulationEngine();
    m_engine->moveToThread(&m_simulationThread);

    connect(&m_simulationThread, &QThread::finished, m_engine, &QObject::deleteLater);
    m_simulationThread.start();
}

void SystemController::createConnections()
{
    auto mw = m_mainWindow.get();
    auto cp = mw->getControlPanel();
    auto sc = m_settingsController.get();

    connect(mw->getActionSettings(), &QAction::triggered, sc, &SettingsUiController::showSettingsDialog);
    connect(sc, &SettingsUiController::requestDataLossConfirmation, mw, [mw](int currentSize, int newSize, bool* ok) {
        QMessageBox::StandardButtons reply;
        reply = QMessageBox::question(mw, "데이터 축소 경고",
                                      QString("데이터 최대 개수를 %1개에서 %2개로 줄이면 이전 데이터 일부가 영구적으로 삭제됩니다. \n\n계속하시겠습니까?").arg(currentSize).arg(newSize),
                                      QMessageBox::Yes | QMessageBox::No);

        *ok = (reply == QMessageBox::Yes);
    }, Qt::BlockingQueuedConnection);

    // ControlPanel -> Engine (start/stop)
    connect(cp, &ControlPanel::startStopClicked, m_engine, [this]() {
        QMetaObject::invokeMethod(m_engine, [this]() {
            if(m_engine->isRunning()) m_engine->stop();
            else m_engine->start();
        });
    });

    // ControlPanel -> Controller
    connect(cp, &ControlPanel::settingsClicked, sc, &SettingsUiController::showSettingsDialog);
    connect(cp, &ControlPanel::amplitudeChanged, sc, &SettingsUiController::onAmplitudeChanged);
    connect(cp, &ControlPanel::currentAmplitudeChanged, sc, &SettingsUiController::onCurrentAmplitudeChanged);
    connect(cp, &ControlPanel::frequencyChanged, sc, &SettingsUiController::onFrequencyChanged);
    connect(cp, &ControlPanel::currentPhaseChanged, sc, &SettingsUiController::onCurrentPhaseChanged);
    connect(cp, &ControlPanel::timeScaleChanged, sc, &SettingsUiController::onTimeScaleChanged);
    connect(cp, &ControlPanel::samplingCyclesChanged, sc, &SettingsUiController::onSamplingCyclesChanged);
    connect(cp, &ControlPanel::samplesPerCycleChanged, sc, &SettingsUiController::onSamplesPerCycleChanged);
    connect(cp, &ControlPanel::updateModeChanged, sc, &SettingsUiController::onUpdateModeChanged);
    connect(cp, &ControlPanel::harmonicsSettingsRequested, sc, &SettingsUiController::onHarmonicsSettingsRequested);
    connect(cp, &ControlPanel::trackingToggled, sc, &SettingsUiController::onTrackingToggled);

    // autoscroll 및 visibility
    connect(cp, &ControlPanel::autoScrollToggled, mw->getGraphWindow(), &GraphWindow::toggleAutoScroll);
    connect(cp, &ControlPanel::autoScrollToggled, mw->getAnalysisGraphWindow(), &AnalysisGraphWindow::toggleAutoScroll);
    connect(cp, &ControlPanel::autoScrollToggled, mw->getFundamentalGraphWindow(), &FundamentalAnalysisGraphWindow::toggleAutoScroll);
    connect(cp, &ControlPanel::autoScrollToggled, mw->getHarmonicGraphWindow(), &HarmonicAnalysisGraphWindow::toggleAutoScroll);
    connect(cp, &ControlPanel::waveformVisibilityChanged, mw->getGraphWindow(), &GraphWindow::onWaveformVisibilityChanged);
    connect(cp, &ControlPanel::analysisWaveformVisibilityChanged, mw->getAnalysisGraphWindow(), &AnalysisGraphWindow::onWaveformVisibilityChanged);
    connect(cp, &ControlPanel::phasorVisibilityChanged, mw->getPhasorView(), &PhasorView::onVisibilityChanged);

    // Dialogs -> Controller
    connect(mw->getThreePhaseDialog(), &ThreePhaseDialog::valueChanged, sc, &SettingsUiController::onThreePhaseValueChanged);
    connect(mw->getPidTuningDialog(), &PidTuningDialog::settingsApplied, sc, &SettingsUiController::onCoefficientsChanged);

    // Controller -> Engine
    connect(sc, &SettingsUiController::setAmplitude, &m_engine->m_amplitude, &Property<double>::setValue);
    connect(sc, &SettingsUiController::setCurrentAmplitude, &m_engine->m_currentAmplitude, &Property<double>::setValue);
    connect(sc, &SettingsUiController::setFrequency, &m_engine->m_frequency, &Property<double>::setValue);
    connect(sc, &SettingsUiController::setCurrentPhase, &m_engine->m_currentPhaseOffsetRadians, &Property<double>::setValue);
    connect(sc, &SettingsUiController::setTimeScale, &m_engine->m_timeScale, &Property<double>::setValue);
    connect(sc, &SettingsUiController::setSamplingCycles, &m_engine->m_samplingCycles, &Property<double>::setValue);
    connect(sc, &SettingsUiController::setSamplesPerCycle, &m_engine->m_samplesPerCycle, &Property<int>::setValue);
    connect(sc, &SettingsUiController::setUpdateMode, &m_engine->m_updateMode, &Property<UpdateMode>::setValue);

    connect(sc, &SettingsUiController::setVoltageHarmonics, &m_engine->m_voltageHarmonic, &Property<HarmonicList>::setValue);
    connect(sc, &SettingsUiController::setCurrentHarmonics, &m_engine->m_currentHarmonic, &Property<HarmonicList>::setValue);

    connect(sc, &SettingsUiController::setVoltageBAmplitude, &m_engine->m_voltage_B_amplitude, &Property<double>::setValue);
    connect(sc, &SettingsUiController::setVoltageBPhase, &m_engine->m_voltage_B_phase_deg, &Property<double>::setValue);
    connect(sc, &SettingsUiController::setVoltageCAmplitude, &m_engine->m_voltage_C_amplitude, &Property<double>::setValue);
    connect(sc, &SettingsUiController::setVoltageCPhase, &m_engine->m_voltage_C_phase_deg, &Property<double>::setValue);
    connect(sc, &SettingsUiController::setCurrentBAmplitude, &m_engine->m_current_B_amplitude, &Property<double>::setValue);
    connect(sc, &SettingsUiController::setCurrentBPhase, &m_engine->m_current_B_phase_deg, &Property<double>::setValue);
    connect(sc, &SettingsUiController::setCurrentCAmplitude, &m_engine->m_current_C_amplitude, &Property<double>::setValue);
    connect(sc, &SettingsUiController::setCurrentCPhase, &m_engine->m_current_C_phase_deg, &Property<double>::setValue);

    connect(sc, &SettingsUiController::setGraphWidth, &m_engine->m_graphWidthSec, &Property<double>::setValue);

    connect(sc, &SettingsUiController::requestCaptureIntervalUpdate, m_engine, &SimulationEngine::recalculateCaptureInterval);
    connect(sc, &SettingsUiController::setMaxDataSize, m_engine, &SimulationEngine::onMaxDataSizeChanged);
    connect(sc, &SettingsUiController::enableTracking, m_engine, &SimulationEngine::enableFrequencyTracking);
    connect(sc, &SettingsUiController::setFrequencyTrackerCoefficients, m_engine, &SimulationEngine::updateFrequencyTrackerCoefficients);

    // Engine -> UI (Property Update)
    connect(static_cast<PropertySignals*>(&m_engine->m_amplitude), static_cast<void (PropertySignals::*)(const double&)>(&PropertySignals::valueChanged), cp, &ControlPanel::setAmplitude);
    connect(static_cast<PropertySignals*>(&m_engine->m_currentAmplitude), static_cast<void (PropertySignals::*)(const double&)>(&PropertySignals::valueChanged), cp, &ControlPanel::setCurrentAmplitude);
    connect(static_cast<PropertySignals*>(&m_engine->m_frequency), static_cast<void (PropertySignals::*)(const double&)>(&PropertySignals::valueChanged), cp, &ControlPanel::setFrequency);
    connect(static_cast<PropertySignals*>(&m_engine->m_timeScale), static_cast<void (PropertySignals::*)(const double&)>(&PropertySignals::valueChanged), cp, &ControlPanel::setTimeScale);
    connect(static_cast<PropertySignals*>(&m_engine->m_samplingCycles), static_cast<void (PropertySignals::*)(const double&)>(&PropertySignals::valueChanged), cp, &ControlPanel::setSamplingCycles);
    connect(static_cast<PropertySignals*>(&m_engine->m_samplesPerCycle), static_cast<void (PropertySignals::*)(const int&)>(&PropertySignals::valueChanged), cp, &ControlPanel::setSamplesPerCycle);
    connect(static_cast<PropertySignals*>(&m_engine->m_updateMode), static_cast<void (PropertySignals::*)(const UpdateMode&)>(&PropertySignals::valueChanged), cp, &ControlPanel::setUpdateMode);
    connect(static_cast<PropertySignals*>(&m_engine->m_currentPhaseOffsetRadians), static_cast<void (PropertySignals::*)(const double&)>(&PropertySignals::valueChanged), mw, [cp](const double& radians) {
        cp->setCurrentPhase(qRound(utils::radiansToDegrees(radians)));
    });

    // Engine -> UI (Graph & Data Update)
    connect(m_engine, &SimulationEngine::dataUpdated, mw->getGraphWindow(), &GraphWindow::updateGraph);
    connect(m_engine, &SimulationEngine::runningStateChanged, cp, &ControlPanel::setRunningState);
    connect(m_engine, &SimulationEngine::measuredDataUpdated, mw->getAnalysisGraphWindow(), &AnalysisGraphWindow::updateGraph);
    connect(m_engine, &SimulationEngine::phasorUpdated, mw->getPhasorView(), &PhasorView::updateData);
    connect(m_engine, &SimulationEngine::measuredDataUpdated, mw->getFundamentalGraphWindow(), &FundamentalAnalysisGraphWindow::updateGraph);
    connect(m_engine, &SimulationEngine::measuredDataUpdated, mw->getHarmonicGraphWindow(), &HarmonicAnalysisGraphWindow::updateGraph);
    connect(m_engine, &SimulationEngine::oneSecondDataUpdated, mw->getOneSecondWindow(), &OneSecondSummaryWindow::updateData);
    connect(m_engine, &SimulationEngine::oneSecondDataUpdated, mw->getAdditionalMetricsWindow(), &AdditionalMetricsWindow::updateData);
    connect(m_engine, &SimulationEngine::oneSecondDataUpdated, mw->getA3700Window(), &A3700N_Window::updateSummaryData);
    connect(m_engine, &SimulationEngine::oneSecondDataUpdated, mw->getDemandCalculator(), &DemandCalculator::processOneSecondData);
    connect(mw->getDemandCalculator(), &DemandCalculator::demandDataUpdated, mw->getA3700Window(), &A3700N_Window::updateDemandData);

    // Graph -> UI (Hover)
    connect(mw->getGraphWindow(), &GraphWindow::redrawNeeded, m_engine, &SimulationEngine::onRedrawRequest);
    connect(mw->getAnalysisGraphWindow(), &AnalysisGraphWindow::redrawNeeded, m_engine, &SimulationEngine::onRedrawAnalysisRequest);
    connect(mw->getFundamentalGraphWindow(), &FundamentalAnalysisGraphWindow::redrawNeeded, m_engine, &SimulationEngine::onRedrawAnalysisRequest);
    connect(mw->getHarmonicGraphWindow(), &HarmonicAnalysisGraphWindow::redrawNeeded, m_engine, &SimulationEngine::onRedrawAnalysisRequest);

    connect(sc, &SettingsUiController::presetApplied, this, [this]() {
        m_mainWindow->onPresetLoaded(m_settingsController->getState());
    });

}
