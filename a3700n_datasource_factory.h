#ifndef A37__N_DATASOURCE_FACTORY_H
#define A37__N_DATASOURCE_FACTORY_H

#include "data_page.h"
class DataSourceFactory
{
public:
    // --- voltage pages ---
    static DataSource createVoltageRmsLLSource();
    static DataSource createVoltageRmsLNSource();
    static DataSource createVoltageFundamentalLLSource();
    static DataSource createVoltageFundamentalLNSource();
    static DataSource createVoltageThdLNSource();
    static DataSource createVoltageThdLLSource();
    static DataSource createVoltageFrequencySource();
    static DataSource createVoltageResidualSource();
    static DataSource createVoltageSymmetricalLLSource();
    static DataSource createVoltageSymmetricalLNSource();
    static DataSource createVoltageUnbalanceSource();

    // --- Current Pages ---
    static DataSource createCurrentRmsSource();
    static DataSource createCurrentFundamentalSource();
    static DataSource createCurrentThdSource();
    static DataSource createCurrentResidualSource();
    static DataSource createCurrentSymmetricalSource();
    static DataSource createCurrentUnbalanceSource();

    // --- Power Pages ---
    static DataSource createPowerActiveSource();
    static DataSource createPowerReactiveSource();
    static DataSource createPowerApparentSource();
    static DataSource createPowerFactorSource();
    static DataSource createPowerEnergySource();

private:
    // --- Extractor 생성 헬퍼 ---

    // 단일 필드에 대한 Extractor 3종(기본, Max, Min)을 생성하는 헬퍼
    template <typename SourceType, typename DemandType>
    static void addExtractors(
        DataSource& dataSource,
        SourceType OneSecondSummaryData::* source,
        MinMaxTracker<DemandType> DemandData::* demand);

    // Max-Only 단일 항목
    template <typename SourceType, typename DemandType>
    static void addMaxOnlyExtractors(
        DataSource& dataSource,
        SourceType OneSecondSummaryData::* source,
        MinMaxTracker<DemandType> DemandData::* demand);

    // 3상 그룹에 대한 Extractor 3종을 생성하는 헬퍼
    template <typename T>
    static void addPhaseGroupExtractors(
        DataSource& dataSource,
        const PhaseData OneSecondSummaryData::* sourceGroup,
        const GenericPhaseData<MinMaxTracker<T>> DemandData::* demandGroup);

    // 3상 그룹 (MaxTracker)
    template <typename T>
    static void addPhaseGroupMaxOnlyExtractors(
        DataSource& dataSource,
        const PhaseData OneSecondSummaryData::* sourceGroup,
        const GenericPhaseData<MaxTracker<T>> DemandData::* demandGroup);

    // LL 그룹에 대한 Extractor 3종을 생성하는 헬퍼
    template <typename T>
    static void addLinetoLineGroupExtractors(
        DataSource& dataSource,
        const LineToLineData OneSecondSummaryData::* sourceGroup,
        const GenericLinetoLineData<MinMaxTracker<T>> DemandData::* demandGroup);

    // LL 그룹 (MaxTracker)
    template <typename T>
    static void addLinetoLineGroupMaxOnlyExtractors(
        DataSource& dataSource,
        const LineToLineData OneSecondSummaryData::* sourceGroup,
        const GenericLinetoLineData<MaxTracker<T>> DemandData::* demandGroup);

    // Average 항목에 대한 Extractor 3종 생성 헬퍼
    template <typename T>
    static void addAverageExtractors(
        DataSource& ds,
        std::function<T(const OneSecondSummaryData&)> avgCalculator,
        const MinMaxTracker<T> DemandData::* demandMember);

};

#endif // A37__N_DATASOURCE_FACTORY_H
