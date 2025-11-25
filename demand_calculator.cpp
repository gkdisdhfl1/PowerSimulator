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
    bindLinetoLineGroup(&DemandData::totalVoltageRms_ll, &OneSecondSummaryData::totalVoltageRms_ll);
    m_mappings.push_back([](DemandData& d, const OneSecondSummaryData& s, const QDateTime& t) {
        const double avg = (s.totalVoltageRms.a + s.totalVoltageRms.b + s.totalVoltageRms.c) / 3.0;
        d.averageTotalVoltageRms.update(avg, t);
        const double avg_ll = (s.totalVoltageRms_ll.ab + s.totalVoltageRms_ll.bc + s.totalVoltageRms_ll.ca) / 3.0;
        d.averageTotalVoltageRms_ll.update(avg_ll, t);
    });

    // 전압 기본파
    m_mappings.push_back([](DemandData& d, const OneSecondSummaryData& s, const QDateTime& t) {
        d.fundamentalVoltageRMS.a.update(s.fundamentalVoltage.a.rms, t);
        d.fundamentalVoltageRMS.b.update(s.fundamentalVoltage.b.rms, t);
        d.fundamentalVoltageRMS.c.update(s.fundamentalVoltage.c.rms, t);
        const double avg = (s.fundamentalVoltage.a.rms + s.fundamentalVoltage.b.rms + s.fundamentalVoltage.c.rms) / 3.0;
        d.averageFundamentalVoltageRms.update(avg, t);
    });
    m_mappings.push_back([](DemandData& d, const OneSecondSummaryData& s, const QDateTime& t) {
        d.fundamentalVoltageRMS_ll.ab.update(s.fundamentalVoltage_ll.ab.rms, t);
        d.fundamentalVoltageRMS_ll.bc.update(s.fundamentalVoltage_ll.bc.rms, t);
        d.fundamentalVoltageRMS_ll.ca.update(s.fundamentalVoltage_ll.ca.rms, t);
        const double avg = (s.fundamentalVoltage_ll.ab.rms + s.fundamentalVoltage_ll.bc.rms + s.fundamentalVoltage_ll.ca.rms) / 3.0;
        d.averageFundamentalVoltageRms_ll.update(avg, t);
    });

    // 전류 RMS
    bindPhaseGroup(&DemandData::totalCurrentRms, &OneSecondSummaryData::totalCurrentRms);
    m_mappings.push_back([](DemandData& d, const OneSecondSummaryData& s, const QDateTime& t) {
        const double avg = (s.totalCurrentRms.a + s.totalCurrentRms.b + s.totalCurrentRms.c) / 3.0;
        d.averageTotalCurrentRms.update(avg, t);
    });

    // 전류 기본파
    m_mappings.push_back([](DemandData& d, const OneSecondSummaryData& s, const QDateTime& t) {
        d.fundamentalCurrentRMS.a.update(s.fundamentalCurrent.a.rms, t);
        d.fundamentalCurrentRMS.b.update(s.fundamentalCurrent.b.rms, t);
        d.fundamentalCurrentRMS.c.update(s.fundamentalCurrent.c.rms, t);
        const double avg = (s.fundamentalCurrent.a.rms + s.fundamentalCurrent.b.rms + s.fundamentalCurrent.c.rms) / 3.0;
        d.averageFundamentalCurrentRms.update(avg, t);
    });

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
    m_mappings.push_back([](DemandData& d, const OneSecondSummaryData& s, const QDateTime& t) {
        d.voltageSymmetricalPositive.update(s.voltageSymmetricalComponents.positive.magnitude, t);
        d.voltageSymmetricalNegative.update(s.voltageSymmetricalComponents.negative.magnitude, t);
        d.voltageSymmetricalZero.update(s.voltageSymmetricalComponents.zero.magnitude, t);

        d.currentSymmetricalPositive.update(s.currentSymmetricalComponents.positive.magnitude, t);
        d.currentSymmetricalNegative.update(s.currentSymmetricalComponents.negative.magnitude, t);
        d.currentSymmetricalZero.update(s.currentSymmetricalComponents.zero.magnitude, t);

        d.voltageSymmetricalPositive_ll.update(s.voltageSymmetricalComponents_ll.positive.magnitude, t);
        d.voltageSymmetricalNegative_ll.update(s.voltageSymmetricalComponents_ll.negative.magnitude, t);
    });

    // 불평형률
    bindMaxOnly(&DemandData::nemaVoltageUnbalance_ll, &OneSecondSummaryData::nemaVoltageUnbalance_ll);
    bindMaxOnly(&DemandData::nemaVoltageUnbalance, &OneSecondSummaryData::nemaVoltageUnbalance);
    bindMaxOnly(&DemandData::voltageU2Unbalance, &OneSecondSummaryData::voltageU2Unbalance);
    bindMaxOnly(&DemandData::voltageU0Unbalance, &OneSecondSummaryData::voltageU0Unbalance);
    bindMaxOnly(&DemandData::nemaCurrentUnbalance, &OneSecondSummaryData::nemaCurrentUnbalance);
    bindMaxOnly(&DemandData::currentU2Unbalance, &OneSecondSummaryData::currentU2Unbalance);
    bindMaxOnly(&DemandData::currentU0Unbalance, &OneSecondSummaryData::currentU0Unbalance);
}
