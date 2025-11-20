#ifndef DEMAND_DATA_H
#define DEMAND_DATA_H

#include "min_max_tracker.h"

template <typename T>
struct GenericPhaseData {
    T a;
    T b;
    T c;
};

template <typename T>
struct GenericLinetoLineData {
    T ab;
    T bc;
    T ca;
};

// Max/Min 추적 대상이 되는 모든 계측 항목들
struct DemandData
{
    // RMS
    GenericPhaseData<MinMaxTracker<double>> totalVoltageRms;
    GenericLinetoLineData<MinMaxTracker<double>> totalVoltageRms_ll;
    MinMaxTracker<double> averageTotalVoltageRms;
    MinMaxTracker<double> averageTotalVoltageRms_ll;

    GenericPhaseData<MinMaxTracker<double>> totalCurrentRms;
    MinMaxTracker<double> averageTotalCurrentRms;

    // Fund
    GenericPhaseData<MinMaxTracker<double>> fundamentalVoltageRMS;
    GenericLinetoLineData<MinMaxTracker<double>> fundamentalVoltageRMS_ll;
    MinMaxTracker<double> averageFundamentalVoltageRms;
    MinMaxTracker<double> averageFundamentalVoltageRms_ll;

    GenericPhaseData<MinMaxTracker<double>> fundamentalCurrentRMS;
    MinMaxTracker<double> averageFundamentalCurrentRms;

    // 주파수
    MinMaxTracker<double> frequency;

    // Residual
    MinMaxTracker<double> voltageResidualRms;
    MinMaxTracker<double> voltageResidualFundamental;
    MinMaxTracker<double> currentResidualRms;
    MinMaxTracker<double> currentResidualFundamental;

    // 전력
    GenericPhaseData<MinMaxTracker<double>> activePower;
    GenericPhaseData<MinMaxTracker<double>> reactivePower;
    GenericPhaseData<MinMaxTracker<double>> apparentPower;
    GenericPhaseData<MinMaxTracker<double>> powerFactor;
    MinMaxTracker<double> totalActivePower;
    MinMaxTracker<double> totalReactivePower;
    MinMaxTracker<double> totalApparentPower;
    MinMaxTracker<double> totalPowerFactor;

    //  --- Max만 추가 ---
    // THD
    GenericPhaseData<ValueWithTimestamp<double>> voltageThd;
    GenericLinetoLineData<ValueWithTimestamp<double>> voltageThd_ll;
    GenericPhaseData<ValueWithTimestamp<double>> currentThd;

    // 대칭 성분
    ValueWithTimestamp<double> voltageSymmetricalPositive; // V1
    ValueWithTimestamp<double> voltageSymmetricalNegative; // V2
    ValueWithTimestamp<double> voltageSymmetricalZero; // V0
    ValueWithTimestamp<double> currentSymmetricalPositive; // I1
    ValueWithTimestamp<double> currentSymmetricalNegative; // I2
    ValueWithTimestamp<double> currentSymmetricalZero; // I0

    ValueWithTimestamp<double> voltageSymmetricalPositive_ll; // V1_ll
    ValueWithTimestamp<double> voltageSymmetricalNegative_ll; // V2_ll

    // 불평형률
    ValueWithTimestamp<double> nemaVoltageUnbalance_ll;
    ValueWithTimestamp<double> nemaVoltageUnbalance;
    ValueWithTimestamp<double> voltageU2Unbalance; // Negative-Sequence
    ValueWithTimestamp<double> voltageU0Unbalance; // Zero-Sequence

    ValueWithTimestamp<double> nemaCurrentUnbalance;
    ValueWithTimestamp<double> currentU2Unbalance; // Negative-Sequence
    ValueWithTimestamp<double> currentU0Unbalance; // Zero-Sequence
};

#endif // DEMAND_DATA_H
