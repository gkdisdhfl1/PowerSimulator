#include "simulation_engine.h"
#include "analysis_utils.h"
#include "frequency_tracker.h"
#include <QDebug>

SimulationEngine::SimulationEngine()
    : QObject()
    , m_currentPhaseRadians(0.0)
    , m_captureIntervalsNs(0)
    , m_simulationTimeNs(0)
    , m_sampleCounterForUpdate(0)
    , m_oneSecondBlockStartTime(0)
    , m_totalEngeryWh(0.0)
{
    // --- Parameters 구조체의 모든 Property 멤버들을 new로 생성하고, parent로 this 지정 ---
    m_params.amplitude = new Property<double>(config::Source::Amplitude::Default, this);
    m_params.currentAmplitude = new Property<double>(config::Source::Current::DefaultAmplitude, this);
    m_params.frequency = new Property<double>(config::Source::Frequency::Default, this);
    m_params.phaseRadians = new Property<double>(0.0, this);
    m_params.currentPhaseOffsetRadians = new Property<double>(0.0, this);
    m_params.timeScale = new Property<double>(config::TimeScale::Default, this);
    m_params.samplingCycles = new Property<double>(config::Sampling::DefaultSamplingCycles, this);
    m_params.samplesPerCycle = new Property<int>(config::Sampling::DefaultSamplesPerCycle, this);
    m_params.maxDataSize = new Property<int>(config::Simulation::DefaultDataSize, this);
    m_params.graphWidthSec = new Property<double>(config::View::GraphWidth::Default, this);
    m_params.updateMode = new Property<UpdateMode>(UpdateMode::PerSample, this);

    m_params.voltageHarmonic = new Property<HarmonicComponent>({config::Harmonics::DefaultOrder, config::Harmonics::DefaultMagnitude, config::Harmonics::DefaultPhase}, this);
    m_params.currentHarmonic = new Property<HarmonicComponent>({config::Harmonics::DefaultOrder, config::Harmonics::DefaultMagnitude, config::Harmonics::DefaultPhase}, this);

    m_params.voltage_B_amplitude = new Property<double>(config::Source::ThreePhase::DefaultAmplitudeB, this);
    m_params.voltage_B_phase_deg = new Property<double>(config::Source::ThreePhase::DefaultPhaseB_deg, this);
    m_params.voltage_C_amplitude = new Property<double>(config::Source::ThreePhase::DefaultAmplitudeC, this);
    m_params.voltage_C_phase_deg = new Property<double>(config::Source::ThreePhase::DefaultPhaseC_deg, this);
    m_params.current_B_amplitude = new Property<double>(config::Source::ThreePhase::DefaultCurrentAmplitudeB, this);
    m_params.current_B_phase_deg = new Property<double>(config::Source::ThreePhase::DefaultCurrentPhaseB_deg, this);
    m_params.current_C_amplitude = new Property<double>(config::Source::ThreePhase::DefaultCurrentAmplitudeC, this);
    m_params.current_C_phase_deg = new Property<double>(config::Source::ThreePhase::DefaultCurrentPhaseC_deg, this);

    // --- Property의 valueChanged 시그널 내부 슬롯에 연결 ---
    connect(m_params.maxDataSize, qOverload<const int&>(&Property<int>::valueChanged), this, &SimulationEngine::handleMaxDataSizeChange);
    connect(m_params.timeScale, qOverload<const double&>(&Property<double>::valueChanged), this, &SimulationEngine::updateCaptureTimer);
    connect(m_params.samplingCycles, qOverload<const double&>(&Property<double>::valueChanged), this, &SimulationEngine::recalculateCaptureInterval);
    connect(m_params.samplesPerCycle, qOverload<const int&>(&Property<int>::valueChanged), this, &SimulationEngine::recalculateCaptureInterval);

    // --- 나머지 초기화 로직 ---
    using namespace std::chrono_literals;
    m_captureTimer.setTimerType(Qt::PreciseTimer);
    recalculateCaptureInterval(); // m_captureIntervalNs 초기 계산
    connect(&m_captureTimer, &QChronoTimer::timeout, this, &SimulationEngine::captureData);

    // FrequencyTracker 생성 및 시그널 연결
    m_frequencyTracker = std::make_unique<FrequencyTracker>(this, this);
    connect(m_frequencyTracker.get(), &FrequencyTracker::samplingCyclesUpdated, m_params.samplingCycles, qOverload<const double&>(&Property<double>::setValue));
}

// ---- public -----
bool SimulationEngine::isRunning() const { return m_captureTimer.isActive(); }
int SimulationEngine::getDataSize() const { return m_data.size(); }
FrequencyTracker* SimulationEngine::getFrequencyTracker() const { return m_frequencyTracker.get(); }
// -----------------

// ---- public slots ----
void SimulationEngine::start()
{
    if (isRunning()) return;
    qDebug() << "m_amplitude = " << m_params.amplitude << " " << "m_frequency = " << m_params.frequency;
    qDebug() << "m_maxDataSize = " << m_params.maxDataSize << "m_timeScale = " << m_params.timeScale;

    m_captureTimer.start();
    emit runningStateChanged(true);
    qDebug() << "Engine started.";
}

void SimulationEngine::stop()
{
    if (!isRunning()) return;

    m_captureTimer.stop();

    emit runningStateChanged(false);
    qDebug() << "Engine stopped.";
}

void SimulationEngine::onMaxDataSizeChanged(int newSize)
{
    m_params.maxDataSize->setValue(newSize);

    while(m_data.size() > static_cast<size_t>(newSize)) {
        m_data.pop_front();
    }
    while(m_measuredData.size() > static_cast<size_t>(newSize)) {
        m_measuredData.pop_front();
    }

    emit dataUpdated(m_data);
}

void SimulationEngine::updateCaptureTimer()
{
    // 기본 캡처 간격에 시간 비율을 곱해서 실제 타이머 주기를 계산
    const auto scaledIntervalNs = m_captureIntervalsNs * m_params.timeScale->value();

    // QChornoTimer는 std::chrono::duration을 직접 인자로 받음
    m_captureTimer.setInterval(std::chrono::duration_cast<Nanoseconds>(scaledIntervalNs));

    // qDebug() << "m_captureTimer.interval()" << m_captureTimer.interval();
}

void SimulationEngine::recalculateCaptureInterval()
{
    using namespace std::chrono_literals;

    double totalSamplesPerSecond = m_params.samplingCycles->value() * m_params.samplesPerCycle->value();
    // qDebug() << "---------------------------------------";
    // qDebug() << "SimulationEngine::recalculateCaptureInterval()";
    // qDebug() << "---------------------------------------";/*
    // qDebug() << "m_params.samplingCycles : " << m_params.samplingCycles;
    // qDebug() << "m_params.samplesPerCycle : " << m_params.samplesPerCycle;
    // qDebug() << "totalSamplesPerSecond: " << totalSamplesPerSecond;
    // qDebug() << "---------------------------------------";

    // if(totalSamplesPerSecond > config::Sampling::MaxSamplesPerSecond) {
    //     totalSamplesPerSecond = config::Sampling::MaxSamplesPerSecond;
    //     qWarning() << "Sampling rate 이 너무 높음. 최대값으로 조정됨.";
    // }
    if(totalSamplesPerSecond > 0) {
        m_captureIntervalsNs = 1.0s / totalSamplesPerSecond;
        // qDebug() << "m_captureIntervalsMs: " << m_captureIntervalsNs / 1000000;
        // qDebug() << "totalSamplesPerSecond: " << totalSamplesPerSecond;
    } else {
        m_captureIntervalsNs = FpNanoseconds(1.0e9);
    }

    updateCaptureTimer();
}

void SimulationEngine::onRedrawRequest()
{
    // 시뮬레이션이 멈춰있을 때만 전체 데이터 강제 업데이트
    // (예: 자동 스크롤이 꺼진 상태에서 그래프와 상호작용할 때)
    if(!isRunning()) {
        emit dataUpdated(m_data);
        return;
    }

    // 누적된 위상을 보고 Mode에 맞춰 update 요청
    processUpdateByMode(false); // 누적 위상 리셋 안함
}

void SimulationEngine::onRedrawAnalysisRequest()
{
    emit measuredDataUpdated(m_measuredData);
}

void SimulationEngine::enableFrequencyTracking(bool enabled)
{
    if(enabled) {
        m_frequencyTracker->startTracking();
    } else {
        m_frequencyTracker->stopTracking();
    }
}
// -----------------------


// ---- private slots ----
void SimulationEngine::captureData()
{
    PhaseData currentVoltage = calculateCurrentVoltage();
    PhaseData currentAmperage = calculateCurrentAmperage();
    addNewDataPoint(currentVoltage, currentAmperage);


    // 사이클 계산을 위해 버퍼 채우기
    m_cycleSampleBuffer.push_back(m_data.back());
    if(m_cycleSampleBuffer.size() > static_cast<size_t>(m_params.samplesPerCycle->value())) {
        m_cycleSampleBuffer.erase(m_cycleSampleBuffer.begin());
    }

    // 주파수, 위상 자동 추적
    m_frequencyTracker->process(m_data.back(), m_measuredData.empty() ? MeasuredData{} : m_measuredData.back(), m_cycleSampleBuffer);

    // 사이클이 꽉 찼으면 사이클 단위 연산 수행
    if(m_cycleSampleBuffer.size() >= static_cast<size_t>(m_params.samplesPerCycle->value())) {
        calculateCycleData();
    }

    // 다음 스텝을 위해 현재 진행 위상 업데이트
    const FpSeconds timeDelta = m_captureIntervalsNs;
    const double phaseDelta = config::Math::TwoPi * m_params.frequency->value() * timeDelta.count();
    m_currentPhaseRadians = std::fmod(m_currentPhaseRadians + phaseDelta, config::Math::TwoPi);

    // UI 갱신 및 사이클 계산을 위한 누적 위상 업데이트
    ++m_sampleCounterForUpdate;

    // 누적된 위상을 보고 Mode에 맞춰 업데이트
    processUpdateByMode(true); // 누적 위상 리셋
    advanceSimulationTime();
}

void SimulationEngine::handleMaxDataSizeChange(int newSize)
{
    while(m_data.size() > static_cast<size_t>(newSize)) {
        m_data.pop_front();
    }
    while(m_measuredData.size() > static_cast<size_t>(newSize)) {
        m_measuredData.pop_front();
    }
    emit dataUpdated(m_data);
}
// -----------------------


// ---- private 함수들 ----
void SimulationEngine::advanceSimulationTime()
{
    m_simulationTimeNs += std::chrono::duration_cast<Nanoseconds>(m_captureIntervalsNs);
    // qDebug() << "m_simulationTimeNs: " << m_simulationTimeNs;

}

PhaseData SimulationEngine::calculateCurrentVoltage() const
{
    PhaseData result;
    const double fundamentalPhase = m_currentPhaseRadians + m_params.phaseRadians->value();
    const auto& harmonic = m_params.voltageHarmonic->value();
    const double harmonicPhaseOffset = utils::degreesToRadians(harmonic.phase) ;

    // 기본파, 고조파 계산
    // A상
    result.a = m_params.amplitude->value() * sin(fundamentalPhase);
    if(harmonic.magnitude > 0.0) {
        const double harmonicPhase = harmonic.order * fundamentalPhase + harmonicPhaseOffset;
        result.a += harmonic.magnitude * sin(harmonicPhase);
    }

    // B상
    const double phase_B_offset = utils::degreesToRadians(m_params.voltage_B_phase_deg->value());
    const double fundamentalPhase_B = fundamentalPhase + phase_B_offset;

    result.b = m_params.voltage_B_amplitude->value() * sin(fundamentalPhase_B);
    if(harmonic.magnitude > 0.0) {
        const double harmonicPhase = harmonic.order * fundamentalPhase_B + harmonicPhaseOffset;
        result.b += harmonic.magnitude * sin(harmonicPhase);
    }

    // C상
    const double phase_C_offset = utils::degreesToRadians(m_params.voltage_C_phase_deg->value());
    const double fundamentalPhase_C = fundamentalPhase + phase_C_offset;

    result.c = m_params.voltage_C_amplitude->value() * sin(fundamentalPhase_C);
    if(harmonic.magnitude > 0.0) {
        const double harmonicPhase = harmonic.order * fundamentalPhase_C + harmonicPhaseOffset;
        result.c += harmonic.magnitude * sin(harmonicPhase);
    }

    return result;
}

PhaseData SimulationEngine::calculateCurrentAmperage() const
{
    PhaseData result;
    const double baseCurrentPhase = m_currentPhaseRadians + m_params.phaseRadians->value() + m_params.currentPhaseOffsetRadians->value();
    const auto& harmonic = m_params.currentHarmonic->value();
    const double harmonicPhaseOffset = utils::degreesToRadians(harmonic.phase) ;

    // 기본파, 고조파 계산
    // A상
    result.a = m_params.currentAmplitude->value() * sin(baseCurrentPhase);
    if(harmonic.magnitude > 0.0) {
        const double harmonicPhase = harmonic.order * baseCurrentPhase + harmonicPhaseOffset;
        result.a += harmonic.magnitude * sin(harmonicPhase);
    }

    // B상
    const double phase_B_offset = utils::degreesToRadians(m_params.current_B_phase_deg->value());
    const double currentSettignsFetched = baseCurrentPhase + phase_B_offset;

    result.b = m_params.current_B_amplitude->value() * sin(currentSettignsFetched);
    if(harmonic.magnitude > 0.0) {
        const double harmonicPhase = harmonic.order * currentSettignsFetched + harmonicPhaseOffset;
        result.b += harmonic.magnitude * sin(harmonicPhase);
    }

    // C상
    const double phase_C_offset = utils::degreesToRadians(m_params.current_C_phase_deg->value());
    const double fundamentalPhase_C = baseCurrentPhase + phase_C_offset;

    result.c = m_params.current_C_amplitude->value() * sin(fundamentalPhase_C);
    if(harmonic.magnitude > 0.0) {
        const double harmonicPhase = harmonic.order * fundamentalPhase_C + harmonicPhaseOffset;
        result.c += harmonic.magnitude * sin(harmonicPhase);
    }

    return result;
}

void SimulationEngine::addNewDataPoint(PhaseData voltage, PhaseData current)
{
    // DataPoint 객체를 생성하여 저장
    // qDebug() << "m_simulationTimeNs: " << m_simulationTimeNs;
    // qDebug() << "voltage: " << voltage;
    // qDebug() << "current: " << current;
    m_data.push_back({m_simulationTimeNs, voltage, current});

    // 최대 개수 관리
    if(m_data.size() > static_cast<size_t>(m_params.maxDataSize->value())) {
        // qDebug() << " ---- data{" << m_simulationTimeNs << ", " << voltage << "} 삭제 ----";
        m_data.pop_front();
    }

}

void SimulationEngine::calculateCycleData()
{
    if(m_cycleSampleBuffer.empty())
        return;

    MeasuredData newData;
    newData.timestamp = m_simulationTimeNs;

    // 1. for 루프를 사용하여 3상에 대한 스펙트럼과 고조파 분석 수행
    for(int i{0}; i < 3; ++i) {
        // --- 전압 분석 ---
        auto voltageSpectrumResult = analyzeSpectrum(AnalysisUtils::DataType::Voltage, i);
        if(voltageSpectrumResult) {
            auto harmonics = AnalysisUtils::findSignificantHarmonics(*voltageSpectrumResult);
            if(i == 0) newData.voltageHarmonics = harmonics;
            else if(i == 1) newData.voltageHarmonicsB = harmonics;
            else newData.voltageHarmonicsC = harmonics;

            if(const auto* fund = AnalysisUtils::getHarmonicComponent(harmonics, 1)) {
                newData.fundamentalVoltage[i] = *fund;
            }
            if(const auto* dom = AnalysisUtils::getDominantHarmonic(harmonics)) {
                newData.dominantVoltage[i] = *dom;
            }
        } else {
            qWarning() << "Voltage Spectrum Analyze Failed !!!";
        }

        // --- 전류 분석 ---
        auto currentSpectrumResult = analyzeSpectrum(AnalysisUtils::DataType::Current, i);
        if(currentSpectrumResult) {
            auto harmonics = AnalysisUtils::findSignificantHarmonics(*currentSpectrumResult);
            if(i == 0) newData.currentHarmonics = harmonics;
            else if(i == 1) newData.currentHarmonicsB = harmonics;
            else newData.currentHarmonicsC = harmonics;

            if(const auto* fund = AnalysisUtils::getHarmonicComponent(harmonics, 1)) {
                newData.fundamentalCurrent[i] = *fund;
            }
            if(const auto* dom = AnalysisUtils::getDominantHarmonic(harmonics)) {
                newData.dominantCurrent[i] = *dom;
            }
        } else {
            qWarning() << "Current Spectrum Analyze Failed !!!";
        }
    }

    // 2. --- 전체 cycle data 계산 ---
    newData.voltageRms = AnalysisUtils::calculateTotalRms(m_cycleSampleBuffer, AnalysisUtils::DataType::Voltage);
    newData.currentRms = AnalysisUtils::calculateTotalRms(m_cycleSampleBuffer, AnalysisUtils::DataType::Current);
    newData.activePower = AnalysisUtils::calculateActivePower(m_cycleSampleBuffer);
    newData.residualVoltageRms = AnalysisUtils::calculateResidualRms(m_cycleSampleBuffer, AnalysisUtils::DataType::Voltage);
    newData.residualCurrentRms = AnalysisUtils::calculateResidualRms(m_cycleSampleBuffer, AnalysisUtils::DataType::Current);


    // 3. 완성된 데이터를 컨테이너에 추가
    m_measuredData.push_back(newData);

    // 4. 1초 데이터 처리 로직 호출
    processOneSecondData(m_measuredData.back());

    // 5. UI에 업데이트 알림
    emit measuredDataUpdated(m_measuredData);

    // 6. 버퍼 비우기
    m_cycleSampleBuffer.clear();
}

void SimulationEngine::processUpdateByMode(bool resetCounter)
{
    bool shouldEmitUpdate = false;
    const int samplesPerCycle = m_params.samplesPerCycle->value();
    const UpdateMode updateMode = m_params.updateMode->value();

    switch (updateMode) {
    case UpdateMode::PerSample:
        shouldEmitUpdate = true;
        break;
    case UpdateMode::PerHalfCycle:
        // 샘플 카운터가 한 사이클을 넘으면 업데이트
        if(m_sampleCounterForUpdate >= samplesPerCycle / 2) {
            shouldEmitUpdate = true;
        }
        break;
    case UpdateMode::PerCycle:
        // 샘플 카운터가 한 사이클을 넘으면 업데이트
        if(m_sampleCounterForUpdate >= samplesPerCycle) {
            shouldEmitUpdate = true;
        }
        break;
    }
    if(shouldEmitUpdate) {
        emit dataUpdated(m_data);
        if(resetCounter) {
            // 사용된 만큼만 카운터를 빼서 오차를 줄임
            if(updateMode == UpdateMode::PerHalfCycle)
                m_sampleCounterForUpdate -= samplesPerCycle / 2;
            else if(updateMode == UpdateMode::PerCycle)
                m_sampleCounterForUpdate -= samplesPerCycle;
            else
                m_sampleCounterForUpdate = 0;
        }
    }
}

std::expected<std::vector<std::complex<double>>, AnalysisUtils::SpectrumError> SimulationEngine::analyzeSpectrum(AnalysisUtils::DataType type, int phase) const
{
    return AnalysisUtils::calculateSpectrum(m_cycleSampleBuffer, type, phase, false);
}

void SimulationEngine::processOneSecondData(const MeasuredData& latestCycleDta)
{
    m_oneSecondCycleBuffer.push_back(latestCycleDta);

    // 시간 경과 확인
    auto elapsedNs = m_simulationTimeNs - m_oneSecondBlockStartTime;
    // qDebug() << m_simulationTimeNs << " - " << m_oneSecondBlockStartTime << " = " << elapsedNs;

    // 995ms (995,000,000 ns)가 되지 않았으면 함수 종료
    if(elapsedNs.count() < 995'000'000LL)
        return;

    // qDebug() << "1초 경과";

    // 1초 데이터 가공 시작
    OneSecondSummaryData summary = AnalysisUtils::buildOneSecondSummary(m_oneSecondCycleBuffer);

    // 누적 전력량 계산
    const double elapsedSeconds = std::chrono::duration_cast<FpSeconds>(elapsedNs).count();
    double totalActivePower = summary.activePower.a + summary.activePower.b + summary.activePower.c;
    m_totalEngeryWh += (totalActivePower * elapsedSeconds) / 3600.0;
    summary.totalEnergyWh = m_totalEngeryWh;

    // 시그널 발생
    emit oneSecondDataUpdated(summary);

    // 다음 1초를 위해 버퍼와 시작 시간 초기화
    m_oneSecondCycleBuffer.clear();

    m_oneSecondBlockStartTime += elapsedNs;
}
