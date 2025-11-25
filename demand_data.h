#ifndef DEMAND_DATA_H
#define DEMAND_DATA_H

#include "min_max_tracker.h"
#include "shared_data_types.h"

// template <typename T>
// struct GenericPhaseData {
//     T a;
//     T b;
//     T c;
// };

// template <typename T>
// struct GenericLinetoLineData {
//     T ab;
//     T bc;
//     T ca;
// };

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
    GenericPhaseData<MaxTracker<double>> voltageThd;
    GenericLinetoLineData<MaxTracker<double>> voltageThd_ll;
    GenericPhaseData<MaxTracker<double>> currentThd;

    // 대칭 성분
    MaxTracker<double> voltageSymmetricalPositive; // V1
    MaxTracker<double> voltageSymmetricalNegative; // V2
    MaxTracker<double> voltageSymmetricalZero; // V0
    MaxTracker<double> currentSymmetricalPositive; // I1
    MaxTracker<double> currentSymmetricalNegative; // I2
    MaxTracker<double> currentSymmetricalZero; // I0

    MaxTracker<double> voltageSymmetricalPositive_ll; // V1_ll
    MaxTracker<double> voltageSymmetricalNegative_ll; // V2_ll

    // 불평형률
    MaxTracker<double> nemaVoltageUnbalance_ll;
    MaxTracker<double> nemaVoltageUnbalance;
    MaxTracker<double> voltageU2Unbalance; // Negative-Sequence
    MaxTracker<double> voltageU0Unbalance; // Zero-Sequence

    MaxTracker<double> nemaCurrentUnbalance;
    MaxTracker<double> currentU2Unbalance; // Negative-Sequence
    MaxTracker<double> currentU0Unbalance; // Zero-Sequence
};

#endif // DEMAND_DATA_H
