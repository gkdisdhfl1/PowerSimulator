#include "simulation_engine.h"
#include <QDebug>

SimulationEngine::SimulationEngine()
    : QObject()
    , m_currentPhaseRadians(0.0)
    , m_accumulatedPhaseSinceUpdate(0.0)
    , m_captureIntervalsMs(0.0)
    , m_simulationTimeNs(0)
    , m_accumulatedPhaseForCycle(0.0)
{
    using namespace std::chrono_literals;

    m_captureTimer.setTimerType(Qt::PreciseTimer);

    m_captureIntervalsMs = 1.0s / (m_params.samplingCycles * m_params.samplesPerCycle);

    connect(&m_captureTimer, &QTimer::timeout, this, &SimulationEngine::captureData);
    updateCaptureTimer(); // 첫 타이머 간격 설정
}

bool SimulationEngine::isRunning() const { return m_captureTimer.isActive(); }
int SimulationEngine::getDataSize() const { return m_data.size(); }
SimulationEngine::Parameters& SimulationEngine::parameters() { return m_params; };
const SimulationEngine::Parameters& SimulationEngine::parameters() const { return m_params; };

// ---- public slots ----
void SimulationEngine::start()
{
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

void SimulationEngine::captureData()
{
    double currentVoltage = calculateCurrentVoltage();
    double currentAmperage = calculateCurrentAmperage();
    addNewDataPoint(currentVoltage, currentAmperage);

    // 1 사이클 계산을 위한 현재 샘플을 버퍼에 추가
    m_cycleSampleBuffer.push_back(m_data.back());

    // 다음 스텝을 위해 현재 진행 위상 업데이트
    const FpSeconds timeDelta = m_captureIntervalsMs;
    const double phaseDelta = 2.0 * std::numbers::pi * m_params.frequency * timeDelta.count();
    m_currentPhaseRadians = std::fmod(m_currentPhaseRadians + phaseDelta, 2.0 * std::numbers::pi);

    // UI 갱신 및 사이클 계산을 위한 누적 위상 업데이트
    m_accumulatedPhaseSinceUpdate += phaseDelta;
    m_accumulatedPhaseForCycle += phaseDelta;

    // 사이클 계산 로직
    if(m_accumulatedPhaseForCycle >= 2.0 * std::numbers::pi) {
        calculateCycleData(); // 1 사이클이 누적되면 계산 실행
        m_accumulatedPhaseForCycle -= 2.0 * std::numbers::pi; // 누적 위상 초기화
    }

    // 누적된 위상을 보고 Mode에 맞춰 업데이트
    processUpdateByMode(true); // 누적 위상 리셋
    advanceSimulationTime();
}

void SimulationEngine::updateCaptureTimer()
{
    // 기본 캡처 간격에 시간 비율을 곱해서 실제 타이머 주기를 계산
    double scaledIntervalMs = (m_captureIntervalsMs * m_params.timeScale).count();

    // 간격이 0이 되는 것을 방지
    if(scaledIntervalMs < config::Simulation::Timer::MinIntervalMs)
        scaledIntervalMs = config::Simulation::Timer::MinIntervalMs;

    // int 최대값 초과 방지
    const double maxTimerInterval = static_cast<double>(std::numeric_limits<int>::max());
    if(scaledIntervalMs > maxTimerInterval)
        scaledIntervalMs = maxTimerInterval;

    m_captureTimer.setInterval(static_cast<int>(std::round(scaledIntervalMs)));
    qDebug() << "m_captureTimer.interval()" << m_captureTimer.interval();
}


void SimulationEngine::advanceSimulationTime()
{
    m_simulationTimeNs += std::chrono::duration_cast<Nanoseconds>(m_captureIntervalsMs);
    // qDebug() << "m_simulationTimeNs: " << m_simulationTimeNs;

}

double SimulationEngine::calculateCurrentVoltage()
{
    const double finalPhase = m_currentPhaseRadians + m_params.phaseRadians;
    return m_params.amplitude * sin(finalPhase);
}

double SimulationEngine::calculateCurrentAmperage()
{
    const double finalPhase = m_currentPhaseRadians + m_params.phaseRadians + m_params.currentPhaseOffsetRadians;
    return m_params.currentAmplitude * sin(finalPhase);
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

void SimulationEngine::recalculateCaptureInterval()
{
    using namespace std::chrono_literals;

    double totalSamplesPerSecond = m_params.samplingCycles * m_params.samplesPerCycle;
    qDebug() << "totalSamplesPerSecond: " << totalSamplesPerSecond;

    if(totalSamplesPerSecond > config::Sampling::MaxSamplesPerSecond) {
        totalSamplesPerSecond = config::Sampling::MaxSamplesPerSecond;
        qWarning() << "Sampling rate 이 너무 높음. 최대값으로 조정됨.";
    }
    if(totalSamplesPerSecond > 0) {
        m_captureIntervalsMs = 1.0s / totalSamplesPerSecond;
        qDebug() << "m_captureIntervalsMs: " << m_captureIntervalsMs;
    } else {
        m_captureIntervalsMs = FpMilliseconds(1.0e9);
    }

    updateCaptureTimer();
}

void SimulationEngine::onRedrawRequest()
{
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

void SimulationEngine::calculateCycleData()
{
    if(m_cycleSampleBuffer.empty())
        return;

    const size_t N = m_cycleSampleBuffer.size();
    const double two_pi_over_N = 2.0 * std::numbers::pi / N;

    // 1. 데이터 집계
    CycleCalculationData voltageData;
    CycleCalculationData currentData;
    double powerSum = 0.0;


    for(size_t n = 0 ; n < N; ++n) {
        const auto& sample = m_cycleSampleBuffer[n];
        const double angle = two_pi_over_N * n;
        const double cos_angle = std::cos(angle);
        const double sin_angle = std::sin(angle);

        // 전압 데이터 집계
        voltageData.squareSum += sample.voltage * sample.voltage;
        voltageData.phasorX_sum += sample.voltage * cos_angle;
        voltageData.phasorY_sum += sample.voltage * sin_angle;

        // 전류 데이터 집계
        currentData.squareSum += sample.current * sample.current;
        currentData.phasorX_sum += sample.current * cos_angle;
        currentData.phasorY_sum += sample.current * sin_angle;

        // 유효 전력 데이터 집계
        powerSum += sample.voltage * sample.current;        
    }

    // 2. 최종 값 계산
    const double voltageRms = std::sqrt(voltageData.squareSum / N);
    const double currentRms = std::sqrt(currentData.squareSum / N);
    const double activePower = powerSum / N;

    // DFT 정규화( 2/N 곱하기)
    const double normFactor = 2.0 / N;
    const double voltagePhasorX = normFactor * voltageData.phasorX_sum;
    const double voltagePhasorY = normFactor * voltageData.phasorY_sum;
    const double currentPhasorX = normFactor * currentData.phasorX_sum;
    const double currentPhasorY = normFactor * currentData.phasorY_sum;

    // 3. 계산된 데이터 구조체를 담아 컨테이너 추가
    m_measuredData.push_back({
        m_simulationTimeNs, // 현재 시간 (사이클 종료 시점)
        voltageRms,
        currentRms,
        activePower,
        voltagePhasorX,
        voltagePhasorY,
        currentPhasorX,
        currentPhasorY
    });
    if(m_measuredData.size() > static_cast<size_t>(m_params.maxDataSize)) {
        m_measuredData.pop_front();
    }

    // 4. UI에 업데이트 알림
    emit measuredDataUpdated(m_measuredData);

    // 5. 버퍼 비우기
    m_cycleSampleBuffer.clear();
}

void SimulationEngine::processUpdateByMode(bool resetAccumulatedPhase)
{
    bool shouldEmitUpdate = false;
    switch (m_params.updateMode) {
    case UpdateMode::PerSample:
        shouldEmitUpdate = true;
        break;
    case UpdateMode::PerHalfCycle:
        // 누적된 위상이 PI(180도) 이상 변했는지 확인
        if(m_accumulatedPhaseSinceUpdate >= std::numbers::pi) {
            shouldEmitUpdate = true;
        }
        break;
    case UpdateMode::PerCycle:
        // 누적된 위상이 2*PI 이상 변했는지 확인
        if(m_accumulatedPhaseSinceUpdate >= 2.0 * std::numbers::pi) {
            shouldEmitUpdate = true;
        }
        break;
    }
    if(shouldEmitUpdate) {
        emit dataUpdated(m_data);
        if(resetAccumulatedPhase) {
            if(m_params.updateMode == UpdateMode::PerHalfCycle)
                m_accumulatedPhaseSinceUpdate -= std::numbers::pi;
            else if(m_params.updateMode == UpdateMode::PerCycle)
                m_accumulatedPhaseSinceUpdate -= 2.0 * std::numbers::pi;
            else
                m_accumulatedPhaseSinceUpdate = 0.0;
        }
    }
}
