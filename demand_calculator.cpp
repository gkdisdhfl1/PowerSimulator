#include "demand_calculator.h"

DemandCalculator::DemandCalculator(QObject *parent)
    : QObject{parent}
{
    initializeMapping();
}

void DemandCalculator::reset()
{
    m_demandData = DemandData{};
}

void DemandCalculator::processOneSecondData(const OneSecondSummaryData& summary)
{
    const QDateTime now = QDateTime::currentDateTime();

    // 등록된 모든 매핑에 대해 업데이트 실시
    for(const auto& updater : m_mappings) {
        updater(m_demandData, summary, now);
    }
}

void DemandCalculator::initializeMapping()
{
    // --- Min Max 추적 항목 ---

    // 전압 RMS
    bindPhaseGroup(&DemandData::totalVoltageRms, &OneSecondSummaryData::totalVoltageRms);
    bindLinetoLineGroup(&DemandData::totalVoltageRms_ll, &OneSecondSummaryData::totalVoltageRms_ll);
    // todo: average 값들은 별도 계산후 bindMinMax 처리 필요

    // 전압 기본파
    // OneSecondSummaryData에 fundamentalVoltageRms가 없으므로, rms값을 추출하는 커스텀 바인더 필요
    // todo: 커스텀 바인더 구현

    // 전류 RMS
    bindPhaseGroup(&DemandData::totalCurrentRms, &OneSecondSummaryData::totalCurrentRms);
    // todo: average 값들은 별도 계산후 bindMinMax 처리 필요

    // 전류 기본파
    // todo: 커스텀 바인더 구현

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
    // todo: SymmetricalComponents 구조체 멤버에 접근하는 커스텀 바인더 필요

    // 불평형률
    bindMaxOnly(&DemandData::nemaVoltageUnbalance_ll, &OneSecondSummaryData::nemaVoltageUnbalance_ll);
    bindMaxOnly(&DemandData::nemaVoltageUnbalance, &OneSecondSummaryData::nemaVoltageUnbalance);
    bindMaxOnly(&DemandData::voltageU2Unbalance, &OneSecondSummaryData::voltageU2Unbalance);
    bindMaxOnly(&DemandData::voltageU0Unbalance, &OneSecondSummaryData::voltageU0Unbalance);
    bindMaxOnly(&DemandData::nemaCurrentUnbalance, &OneSecondSummaryData::nemaCurrentUnbalance);
    bindMaxOnly(&DemandData::currentU2Unbalance, &OneSecondSummaryData::currentU2Unbalance);
    bindMaxOnly(&DemandData::currentU0Unbalance, &OneSecondSummaryData::currentU0Unbalance);
}
