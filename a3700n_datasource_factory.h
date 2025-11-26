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
    template <typename SourcePtr, typename DemandPtr>
    static void addExtractors(
        DataSource& dataSource,
        SourcePtr source,
        DemandPtr demand);

    // Max-Only 단일 항목
    template <typename SourcePtr, typename DemandPtr>
    static void addMaxOnlyExtractors(
        DataSource& dataSource,
        SourcePtr source,
        DemandPtr demand);

    // 3상 그룹에 대한 Extractor 3종을 생성하는 헬퍼
    template <typename SourceGroupPtr, typename DemandGroupPtr, typename ValueExtractor>
    static void addPhaseGroupExtractors(
            DataSource& dataSource,
            SourceGroupPtr sourceGroup,
            DemandGroupPtr demandGroup,
            ValueExtractor extractor);
    template <typename SourceGroupPtr, typename DemandGroupPtr>
         static void addPhaseGroupExtractors(
            DataSource& dataSource,
            SourceGroupPtr sourceGroup,
            DemandGroupPtr demandGroup);

    // 3상 그룹 (MaxTracker)
    template <typename SourceGroupPtr, typename DemandGroupPtr>
    static void addPhaseGroupMaxOnlyExtractors(
        DataSource& dataSource,
        SourceGroupPtr sourceGroup,
        DemandGroupPtr demandGroup);

    // LL 그룹에 대한 Extractor 3종을 생성하는 헬퍼
    template <typename SourceGroupPtr, typename DemandGroupPtr, typename ValueExtractor>
    static void addLinetoLineGroupExtractors(
        DataSource& dataSource,
        SourceGroupPtr sourceGroup,
        DemandGroupPtr demandGroup,
        ValueExtractor extractor);
    template <typename SourceGroupPtr, typename DemandGroupPtr>
    static void addLinetoLineGroupExtractors(
        DataSource& dataSource,
        SourceGroupPtr sourceGroup,
        DemandGroupPtr demandGroup);


    // LL 그룹 (MaxTracker)
    template <typename SourceGroupPtr, typename DemandGroupPtr>
    static void addLinetoLineGroupMaxOnlyExtractors(
        DataSource& dataSource,
        SourceGroupPtr sourceGroup,
        DemandGroupPtr demandGroup);

    // Average 항목에 대한 Extractor 3종 생성 헬퍼
    template <typename T>
    static void addAverageExtractors(
        DataSource& ds,
        std::function<T(const OneSecondSummaryData&)> avgCalculator,
        const MinMaxTracker<T> DemandData::* demandMember);

    // Symmetrical Components 항목에 대한 Extractor 생성 헬퍼
    template <typename SourceGroupPtr, typename DemandGroupPtr>
    static void addSymmetricalGroupExtractors(
        DataSource& ds, SourceGroupPtr sourceGroup, DemandGroupPtr demandGroup, bool hasZero);

};

#endif // A37__N_DATASOURCE_FACTORY_H
