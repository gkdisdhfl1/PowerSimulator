#include "simulation_engine.h"
#include "AnalysisUtils.h"
#include "frequency_tracker.h"
#include <QDebug>

SimulationEngine::SimulationEngine()
    : QObject()
    , m_currentPhaseRadians(0.0)
    , m_captureIntervalsNs(0)
    , m_simulationTimeNs(0)
    , m_sampleCounterForUpdate(0)
{
    using namespace std::chrono_literals;

    m_captureTimer.setTimerType(Qt::PreciseTimer);

    m_captureIntervalsNs = 1.0s / (m_params.samplingCycles * m_params.samplesPerCycle);

    connect(&m_captureTimer, &QChronoTimer::timeout, this, &SimulationEngine::captureData);
    updateCaptureTimer(); // 첫 타이머 간격 설정

    // FrequencyTracker 생성 및 시그널 연결
    m_frequencyTracker = std::make_unique<FrequencyTracker>(this, this);
    connect(m_frequencyTracker.get(), &FrequencyTracker::samplingCyclesUpdated, this, [this](double newFreq) {
        m_params.samplingCycles = newFreq;
        recalculateCaptureInterval();
        emit samplingCyclesUpdated(newFreq);
    });
}

// ---- public -----
bool SimulationEngine::isRunning() const { return m_captureTimer.isActive(); }
int SimulationEngine::getDataSize() const { return m_data.size(); }
SimulationEngine::Parameters& SimulationEngine::parameters() { return m_params; };
const SimulationEngine::Parameters& SimulationEngine::parameters() const { return m_params; };
FrequencyTracker* SimulationEngine::getFrequencyTracker() const { return m_frequencyTracker.get(); }
// -----------------

// ---- public slots ----
void SimulationEngine::start()
{
    // 필요한 N값 목록을 가져옴
    std::vector<int> required_N_values = m_frequencyTracker->getRequiredValues();
    AnalysisUtils::precomputeTables(required_N_values);

    if (isRunning()) return;
    qDebug() << "시작";
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
    m_params.maxDataSize = newSize;

    while(m_data.size() > static_cast<size_t>(m_params.maxDataSize)) {
        m_data.pop_front();
    }
    while(m_measuredData.size() > static_cast<size_t>(m_params.maxDataSize)) {
        m_measuredData.pop_front();
    }

    emit dataUpdated(m_data);
}

void SimulationEngine::updateCaptureTimer()
{
    // 기본 캡처 간격에 시간 비율을 곱해서 실제 타이머 주기를 계산
    const auto scaledIntervalNs = m_captureIntervalsNs * m_params.timeScale;

    // QChornoTimer는 std::chrono::duration을 직접 인자로 받음
    m_captureTimer.setInterval(std::chrono::duration_cast<Nanoseconds>(scaledIntervalNs));

    // qDebug() << "m_captureTimer.interval()" << m_captureTimer.interval();
}

void SimulationEngine::recalculateCaptureInterval()
{
    using namespace std::chrono_literals;

    double totalSamplesPerSecond = m_params.samplingCycles * m_params.samplesPerCycle;
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
    double currentVoltage = calculateCurrentVoltage();
    double currentAmperage = calculateCurrentAmperage();
    addNewDataPoint(currentVoltage, currentAmperage);


    // 사이클 계산을 위해 버퍼 채우기
    m_cycleSampleBuffer.push_back(m_data.back());
    if(m_cycleSampleBuffer.size() > static_cast<size_t>(m_params.samplesPerCycle)) {
        m_cycleSampleBuffer.erase(m_cycleSampleBuffer.begin());
    }

    m_frequencyTracker->process(m_data.back(), m_measuredData.empty() ? MeasuredData{} : m_measuredData.back(), m_cycleSampleBuffer);

    // 사이클이 꽉 찼으면 사이클 단위 연산 수행
    if(m_cycleSampleBuffer.size() >= static_cast<size_t>(m_params.samplesPerCycle)) {
        calculateCycleData();
    }

    // 다음 스텝을 위해 현재 진행 위상 업데이트
    const FpSeconds timeDelta = m_captureIntervalsNs;
    const double phaseDelta = config::Math::TwoPi * m_params.frequency * timeDelta.count();
    m_currentPhaseRadians = std::fmod(m_currentPhaseRadians + phaseDelta, config::Math::TwoPi);

    // UI 갱신 및 사이클 계산을 위한 누적 위상 업데이트
    ++m_sampleCounterForUpdate;

    // 누적된 위상을 보고 Mode에 맞춰 업데이트
    processUpdateByMode(true); // 누적 위상 리셋
    advanceSimulationTime();
}
// -----------------------


// ---- private 함수들 ----
void SimulationEngine::advanceSimulationTime()
{
    m_simulationTimeNs += std::chrono::duration_cast<Nanoseconds>(m_captureIntervalsNs);
    // qDebug() << "m_simulationTimeNs: " << m_simulationTimeNs;

}

double SimulationEngine::calculateCurrentVoltage() const
{
    // 1. 기본파 계산
    const double fundamentalPhase = m_currentPhaseRadians + m_params.phaseRadians;
    double voltage = m_params.amplitude * sin(fundamentalPhase);

    // 2. 고조파 계산 (크기가 0보다 클 때만)
    const auto& harmonic = m_params.voltageHarmonic;
    if(harmonic.magnitude > 0.0) {
        const double harmonicPhaseOffset = utils::degreesToRadians(harmonic.phase) ;
        const double harmonicPhase = harmonic.order * fundamentalPhase + harmonicPhaseOffset;
        voltage += harmonic.magnitude * sin(harmonicPhase);
    }

    return voltage;
}

double SimulationEngine::calculateCurrentAmperage() const
{
    // 1. 기본파 계산
    const double fundamentalPhase = m_currentPhaseRadians + m_params.phaseRadians + m_params.currentPhaseOffsetRadians;
    double current = m_params.currentAmplitude * sin(fundamentalPhase);

    // 2. 고조파 계산 (크기가 0보다 클 때만)
    const auto& harmonic = m_params.currentHarmonic;
    if(harmonic.magnitude > 0.0) {
        const double harmonicPhaseOffset = utils::degreesToRadians(harmonic.phase) ;
        const double harmonicPhase = harmonic.order * fundamentalPhase + harmonicPhaseOffset;
        current += harmonic.magnitude * sin(harmonicPhase);
    }

    return current;
}

void SimulationEngine::addNewDataPoint(double voltage, double current)
{
    // DataPoint 객체를 생성하여 저장
    // qDebug() << "m_simulationTimeNs: " << m_simulationTimeNs;
    // qDebug() << "voltage: " << voltage;
    // qDebug() << "current: " << current;
    m_data.push_back({m_simulationTimeNs, voltage, current});

    // 최대 개수 관리
    if(m_data.size() > static_cast<size_t>(m_params.maxDataSize)) {
        // qDebug() << " ---- data{" << m_simulationTimeNs << ", " << voltage << "} 삭제 ----";
        m_data.pop_front();
    }

}

void SimulationEngine::calculateCycleData()
{
    if(m_cycleSampleBuffer.empty())
        return;

    // 1. (측정) 파형의 전체 주파수 스펙트럼 분석
    auto voltageSpectrum = analyzeSpectrum(DataType::Voltage);
    auto currentSpectrum = analyzeSpectrum(DataType::Current);

    // 2. (해석) 스펙트럼에서 가장 큰 두 개의 성분(기본파, 고조파)를 찾음
    auto voltageHarmonics = AnalysisUtils::findSignificantHarmonics(voltageSpectrum);
    auto currentHarmonics = AnalysisUtils::findSignificantHarmonics(currentSpectrum);

    // 3. 전체 RMS 및 유효 전력 계산
    double voltageSquareSum = 0.0;
    double currentSquareSum = 0.0;
    double powerSum = 0.0;
    const size_t N = m_cycleSampleBuffer.size();

    for(const auto& sample : m_cycleSampleBuffer) {
        voltageSquareSum += sample.voltage * sample.voltage;
        currentSquareSum += sample.current * sample.current;
        powerSum += sample.voltage * sample.current;
    }

    const double totalVoltageRms = std::sqrt(voltageSquareSum / N);
    const double totalCurrentRms = std::sqrt(currentSquareSum / N);
    const double activePower = powerSum / N;

    // 4. 계산된 데이터 구조체를 담아 컨테이너 추가
    m_measuredData.push_back({
        m_simulationTimeNs, // 현재 시간 (사이클 종료 시점)
        totalVoltageRms,
        totalCurrentRms,
        activePower,
        voltageHarmonics,
        currentHarmonics,
    });

    if(m_measuredData.size() > static_cast<size_t>(m_params.maxDataSize)) {
        m_measuredData.pop_front();
    }

    // 5. UI에 업데이트 알림
    emit measuredDataUpdated(m_measuredData);

    // 6. 버퍼 비우기
    m_cycleSampleBuffer.clear();
}

void SimulationEngine::processUpdateByMode(bool resetCounter)
{
    bool shouldEmitUpdate = false;
    const int samplesPerCycle = m_params.samplesPerCycle;

    switch (m_params.updateMode) {
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
            if(m_params.updateMode == UpdateMode::PerHalfCycle)
                m_sampleCounterForUpdate -= samplesPerCycle / 2;
            else if(m_params.updateMode == UpdateMode::PerCycle)
                m_sampleCounterForUpdate -= samplesPerCycle;
            else
                m_sampleCounterForUpdate = 0;
        }
    }
}

std::vector<std::complex<double>> SimulationEngine::analyzeSpectrum(SimulationEngine::DataType type) const
{
    // 전압 전류에 따라 AnalysisUtils에 넘길 데이터 복사
    std::vector<DataPoint> samplesForAnalysis;
    samplesForAnalysis.reserve(m_cycleSampleBuffer.size());

    for(const auto& sample : m_cycleSampleBuffer) {
        samplesForAnalysis.push_back({
            sample.timestamp,
            (type == DataType::Voltage) ? sample.voltage : sample.current,
            0
        });
    }

    return AnalysisUtils::calculateSpectrum(samplesForAnalysis, false);
}
