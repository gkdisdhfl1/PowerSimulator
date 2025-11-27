#include "demand_calculator.h"

DemandCalculator::DemandCalculator(QObject *parent)
    : QObject{parent}
{
    initializeMapping();
}

void DemandCalculator::processOneSecondData(const OneSecondSummaryData& summary)
{
    const QDateTime now = QDateTime::currentDateTime();

    // 등록된 모든 매핑에 대해 업데이트 실시
    for(const auto& updater : m_mappings) {
        updater(m_demandData, summary, now);
    }

    emit demandDataUpdated(m_demandData);
}

void DemandCalculator::initializeMapping()
{
    // --- Min Max 추적 항목 ---

    // 전압 RMS
    bindPhaseGroup(&DemandData::totalVoltageRms, &OneSecondSummaryData::totalVoltageRms);
    bindPhaseAverage(&DemandData::averageTotalVoltageRms, &OneSecondSummaryData::totalVoltageRms);

    bindLinetoLineGroup(&DemandData::totalVoltageRms_ll, &OneSecondSummaryData::totalVoltageRms_ll);
    bindLinetoLineAverage(&DemandData::averageTotalVoltageRms_ll, &OneSecondSummaryData::totalVoltageRms_ll);

    // 전압 기본파
    auto extractRms = [](const HarmonicAnalysisResult& h) { return h.rms; };

    bindPhaseGroup(&DemandData::fundamentalVoltageRMS, &OneSecondSummaryData::fundamentalVoltage, extractRms);
    bindPhaseAverage(&DemandData::averageFundamentalVoltageRms, &OneSecondSummaryData::fundamentalVoltage, extractRms);

    bindLinetoLineGroup(&DemandData::fundamentalVoltageRMS_ll, &OneSecondSummaryData::fundamentalVoltage_ll, extractRms);
    bindLinetoLineAverage(&DemandData::averageFundamentalVoltageRms_ll, &OneSecondSummaryData::fundamentalVoltage_ll, extractRms);

    // 전류 RMS
    bindPhaseGroup(&DemandData::totalCurrentRms, &OneSecondSummaryData::totalCurrentRms);
    bindPhaseAverage(&DemandData::averageTotalCurrentRms, &OneSecondSummaryData::totalCurrentRms);

    // 전류 기본파
    bindPhaseGroup(&DemandData::fundamentalCurrentRMS, &OneSecondSummaryData::fundamentalCurrent, extractRms);
    bindPhaseAverage(&DemandData::averageFundamentalCurrentRms, &OneSecondSummaryData::fundamentalCurrent, extractRms);

    // 주파수
    bindMinMax(&DemandData::frequency, &OneSecondSummaryData::frequency);

    // Residual
    bindMinMax(&DemandData::voltageResidualRms, &OneSecondSummaryData::residualVoltageRms);
    bindMinMax(&DemandData::voltageResidualFundamental, &OneSecondSummaryData::residualVoltageFundamental);
    bindMinMax(&DemandData::currentResidualRms, &OneSecondSummaryData::residualCurrentRms);
    bindMinMax(&DemandData::currentResidualFundamental, &OneSecondSummaryData::residualCurrentFundamental);

    // 전력 및 역률
    bindPhaseGroup(&DemandData::activePower, &OneSecondSummaryData::activePower);
    bindPhaseGroup(&DemandData::reactivePower, &OneSecondSummaryData::reactivePower);
    bindPhaseGroup(&DemandData::apparentPower, &OneSecondSummaryData::apparentPower);
    bindMinMax(&DemandData::totalActivePower, &OneSecondSummaryData::totalActivePower);
    bindMinMax(&DemandData::totalReactivePower, &OneSecondSummaryData::totalReactivePower);
    bindMinMax(&DemandData::totalApparentPower, &OneSecondSummaryData::totalApparentPower);

    bindPhaseGroup(&DemandData::powerFactor, &OneSecondSummaryData::powerFactor);
    bindMinMax(&DemandData::totalPowerFactor, &OneSecondSummaryData::totalPowerFactor);

    // --- max만 추적 항목 ---

    // THD
    bindPhaseGroupMaxOnly(&DemandData::voltageThd, &OneSecondSummaryData::voltageThd);
    bindLinetoLineGroupMaxOnly(&DemandData::voltageThd_ll, &OneSecondSummaryData::voltageThd_ll);
    bindPhaseGroupMaxOnly(&DemandData::currentThd, &OneSecondSummaryData::currentThd);

    // 대칭 성분
    bindPhaseSymmetricalGroup(&DemandData::voltageSymmetricalComponents, &OneSecondSummaryData::voltageSymmetricalComponents);
    bindLineToLineSymmetricalGroup(&DemandData::voltageSymmetricalComponents_ll, &OneSecondSummaryData::voltageSymmetricalComponents_ll);

    bindPhaseSymmetricalGroup(&DemandData::currentSymmetricalComponents, &OneSecondSummaryData::currentSymmetricalComponents);


    // 불평형률
    bindMaxOnly(&DemandData::nemaVoltageUnbalance_ll, &OneSecondSummaryData::nemaVoltageUnbalance_ll);
    bindMaxOnly(&DemandData::nemaVoltageUnbalance, &OneSecondSummaryData::nemaVoltageUnbalance);
    bindMaxOnly(&DemandData::voltageU2Unbalance, &OneSecondSummaryData::voltageU2Unbalance);
    bindMaxOnly(&DemandData::voltageU0Unbalance, &OneSecondSummaryData::voltageU0Unbalance);
    bindMaxOnly(&DemandData::nemaCurrentUnbalance, &OneSecondSummaryData::nemaCurrentUnbalance);
    bindMaxOnly(&DemandData::currentU2Unbalance, &OneSecondSummaryData::currentU2Unbalance);
    bindMaxOnly(&DemandData::currentU0Unbalance, &OneSecondSummaryData::currentU0Unbalance);
}
