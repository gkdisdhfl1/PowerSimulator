#include "a3700n_datasource_factory.h"

// ==================================
// Private Static 헬퍼 함수들
// ==================================

template <typename SourceGroupMemberPtr, typename DemandGroupMemberPtr>
void DataSourceFactory::addExtractors(DataSource& dataSource,
                                  SourceGroupMemberPtr source,
                                  DemandGroupMemberPtr demand)
{
    // 기본 Extractor
    dataSource.extractors.push_back([source](const OneSecondSummaryData& s) { return s.*source; });

    // Max Extractor
    dataSource.maxExtractors.push_back([demand](const DemandData& d) { return (d.*demand).max; });

    // Min Extractor
    dataSource.minExtractors.push_back([demand](const DemandData& d) { return (d.*demand).min; });
}

template <typename SourcePtr, typename DemandPtr>
void DataSourceFactory::addMaxOnlyExtractors(DataSource& dataSource,
                                      SourcePtr source,
                                      DemandPtr demand)
{
    // 기본 Extractor
    dataSource.extractors.push_back([source](const OneSecondSummaryData& s) { return s.*source; });

    // Max Extractor
    dataSource.maxExtractors.push_back([demand](const DemandData& d) { return (d.*demand); });

}

template <typename SourceGroupPtr, typename DemandGroupPtr, typename ValueExtractor>
void DataSourceFactory::addPhaseGroupExtractors(
    DataSource& dataSource,
    SourceGroupPtr sourceGroup,
    DemandGroupPtr demandGroup,
    ValueExtractor extractor)
{
    // A
    dataSource.extractors.push_back([sourceGroup, extractor](const OneSecondSummaryData& s) { return extractor((s.*sourceGroup).a); });
    dataSource.maxExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).a.max; });
    dataSource.minExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).a.min; });

    // B
    dataSource.extractors.push_back([sourceGroup, extractor](const OneSecondSummaryData& s) { return extractor((s.*sourceGroup).b); });
    dataSource.maxExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).b.max; });
    dataSource.minExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).b.min; });

    // C
    dataSource.extractors.push_back([sourceGroup, extractor](const OneSecondSummaryData& s) { return extractor((s.*sourceGroup).c); });
    dataSource.maxExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).c.max; });
    dataSource.minExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).c.min; });
}
template <typename SourceGroupPtr, typename DemandGroupPtr>
void DataSourceFactory::addPhaseGroupExtractors(
    DataSource& dataSource,
    SourceGroupPtr sourceGroup,
    DemandGroupPtr demandGroup)
{
    addPhaseGroupExtractors(dataSource, sourceGroup, demandGroup, [](const auto& v){ return v; });
}

template <typename SourceGroupPtr, typename DemandGroupPtr>
void DataSourceFactory::addPhaseGroupMaxOnlyExtractors(
    DataSource& dataSource,
    SourceGroupPtr sourceGroup,
    DemandGroupPtr demandGroup)
{
    dataSource.extractors.push_back([sourceGroup](const OneSecondSummaryData& s) { return (s.*sourceGroup).a; });
    dataSource.maxExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).a; });

    // B
    dataSource.extractors.push_back([sourceGroup](const OneSecondSummaryData& s) { return (s.*sourceGroup).b; });
    dataSource.maxExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).b; });

    // C
    dataSource.extractors.push_back([sourceGroup](const OneSecondSummaryData& s) { return (s.*sourceGroup).c; });
    dataSource.maxExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).c; });
}

template <typename SourceGroupPtr, typename DemandGroupPtr, typename ValueExtractor>
void DataSourceFactory::addLinetoLineGroupExtractors(
    DataSource& dataSource,
    SourceGroupPtr sourceGroup,
    DemandGroupPtr demandGroup,
    ValueExtractor extractor)
{
    dataSource.extractors.push_back([sourceGroup, extractor](const OneSecondSummaryData& s) { return extractor((s.*sourceGroup).ab);});
    dataSource.maxExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).ab.max; });
    dataSource.minExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).ab.min; });

    dataSource.extractors.push_back([sourceGroup, extractor](const OneSecondSummaryData& s) { return extractor((s.*sourceGroup).bc);});
    dataSource.maxExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).bc.max; });
    dataSource.minExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).bc.min; });

    dataSource.extractors.push_back([sourceGroup, extractor](const OneSecondSummaryData& s) { return extractor((s.*sourceGroup).ca);});
    dataSource.maxExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).ca.max; });
    dataSource.minExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).ca.min; });
}
template <typename SourceGroupPtr, typename DemandGroupPtr>
void DataSourceFactory::addLinetoLineGroupExtractors(
    DataSource& dataSource,
    SourceGroupPtr sourceGroup,
    DemandGroupPtr demandGroup)
{
    addLinetoLineGroupExtractors(dataSource, sourceGroup, demandGroup, [](const auto& v){ return v; });
}

template <typename SourceGroupPtr, typename DemandGroupPtr>
void DataSourceFactory::addLinetoLineGroupMaxOnlyExtractors(
    DataSource& dataSource,
    SourceGroupPtr sourceGroup,
    DemandGroupPtr demandGroup)
{
    dataSource.extractors.push_back([sourceGroup](const OneSecondSummaryData& s) { return (s.*sourceGroup).ab;});
    dataSource.maxExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).ab; });

    dataSource.extractors.push_back([sourceGroup](const OneSecondSummaryData& s) { return (s.*sourceGroup).bc;});
    dataSource.maxExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).bc; });

    dataSource.extractors.push_back([sourceGroup](const OneSecondSummaryData& s) { return (s.*sourceGroup).ca;});
    dataSource.maxExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).ca; });
}

template <typename T>
void DataSourceFactory::addAverageExtractors(
    DataSource& ds,
    std::function<T(const OneSecondSummaryData&)> avgCalculator,
    const MinMaxTracker<T> DemandData::* demandMember)
{
    ds.extractors.push_back(avgCalculator);
    ds.maxExtractors.push_back([demandMember](const DemandData& d) { return (d.*demandMember).max; });
    ds.minExtractors.push_back([demandMember](const DemandData& d) { return (d.*demandMember).min; });
}

template <typename SourceGroupPtr, typename DemandGroupPtr>
void DataSourceFactory::addSymmetricalGroupExtractors_ll(
    DataSource& ds, SourceGroupPtr sourceGroup, DemandGroupPtr demandGroup)
{
    // Positive
    ds.extractors.push_back([=](const auto& s) { return (s.*sourceGroup).positive.magnitude; });
    ds.maxExtractors.push_back([=](const auto& d) { return (d.*demandGroup).positive; });

    // Negative
    ds.extractors.push_back([=](const auto& s) { return (s.*sourceGroup).negative.magnitude; });
    ds.maxExtractors.push_back([=](const auto& d) { return (d.*demandGroup).negative; });
}
template <typename SourceGroupPtr, typename DemandGroupPtr>
void DataSourceFactory::addSymmetricalGroupExtractors(
    DataSource& ds, SourceGroupPtr sourceGroup, DemandGroupPtr demandGroup)
{
    addSymmetricalGroupExtractors_ll(ds, sourceGroup, demandGroup);

    // Zero
    ds.extractors.push_back([=](const auto& s) { return (s.*sourceGroup).zero.magnitude; });
    ds.maxExtractors.push_back([=](const auto& d) { return (d.*demandGroup).zero; });
}

// ==================================
// Public Static 함수들
// ==================================

// --- Voltage Pages ---
DataSource DataSourceFactory::createVoltageRmsLLSource()
{
    DataSource llSource;
    llSource.name = "L-L";
    llSource.rowLabels = {"AB", "BC", "CA", "Average"};
    addLinetoLineGroupExtractors(llSource, &OneSecondSummaryData::totalVoltageRms_ll, &DemandData::totalVoltageRms_ll);

    // Average
    addAverageExtractors<double>(
        llSource,
        [](const OneSecondSummaryData& s) { return (s.totalVoltageRms_ll.ab + s.totalVoltageRms_ll.bc + s.totalVoltageRms_ll.ca) / 3.0; },
        &DemandData::averageTotalVoltageRms_ll);

    return llSource;
}

DataSource DataSourceFactory::createVoltageRmsLNSource()
{
    DataSource lnSource;
    lnSource.name = "L-N";
    lnSource.rowLabels = {"A", "B", "C", "Average"};
    addPhaseGroupExtractors(lnSource, &OneSecondSummaryData::totalVoltageRms, &DemandData::totalVoltageRms);

    // Average
    addAverageExtractors<double>(
        lnSource,
        [](const OneSecondSummaryData& d) { return (d.totalVoltageRms.a + d.totalVoltageRms.b+ d.totalVoltageRms.c) / 3.0; },
        &DemandData::averageTotalVoltageRms);

    return lnSource;
}

DataSource DataSourceFactory::createVoltageFundamentalLLSource()
{
    DataSource llSource;
    llSource.name = "L-L";
    llSource.rowLabels = {"AB", "BC", "CA", "Average"};

    auto extractRms = [](const HarmonicAnalysisResult &h) { return h.rms; };

    addLinetoLineGroupExtractors(llSource, &OneSecondSummaryData::fundamentalVoltage_ll, &DemandData::fundamentalVoltageRMS_ll, extractRms);
    addAverageExtractors<double>(
        llSource,
        [](const OneSecondSummaryData& s) { return (s.fundamentalVoltage_ll.ab.rms + s.fundamentalVoltage_ll.bc.rms + s.fundamentalVoltage_ll.ca.rms) / 3.0; },
        &DemandData::averageFundamentalVoltageRms_ll
    );

    return llSource;
}

DataSource DataSourceFactory::createVoltageFundamentalLNSource()
{
    DataSource lnSource;
    lnSource.name = "L-N";
    lnSource.rowLabels = {"A", "B", "C", "Average"};

    auto extractRms = [](const HarmonicAnalysisResult &h) { return h.rms; };

    addPhaseGroupExtractors(lnSource, &OneSecondSummaryData::fundamentalVoltage, &DemandData::fundamentalVoltageRMS, extractRms);
    addAverageExtractors<double>(
        lnSource,
        [](const OneSecondSummaryData& s) { return (s.fundamentalVoltage.a.rms + s.fundamentalVoltage.b.rms + s.fundamentalVoltage.c.rms) / 3.0; },
        &DemandData::averageFundamentalVoltageRms
    );

    return lnSource;
}

DataSource DataSourceFactory::createVoltageThdLLSource()
{
    DataSource llSource;
    llSource.name = "L-L";
    llSource.rowLabels = {"AB", "BC", "CA"};

    addLinetoLineGroupMaxOnlyExtractors(llSource, &OneSecondSummaryData::voltageThd_ll, &DemandData::voltageThd_ll);

    return llSource;
}

DataSource DataSourceFactory::createVoltageThdLNSource()
{
    DataSource lnSource;
    lnSource.name = "L-N";
    lnSource.rowLabels = {"A", "B", "C"};

    addPhaseGroupMaxOnlyExtractors(lnSource, &OneSecondSummaryData::voltageThd, &DemandData::voltageThd);

    return lnSource;
}

DataSource DataSourceFactory::createVoltageFrequencySource()
{
    DataSource ds;
    ds.name = "";
    ds.rowLabels = {"Frequency"};
    addExtractors(ds, &OneSecondSummaryData::frequency, &DemandData::frequency);

    return ds;
}

DataSource DataSourceFactory::createVoltageResidualSource()
{
    DataSource ds;
    ds.name = "";
    ds.rowLabels = {"RMS", "Fund."};
    addExtractors(ds, &OneSecondSummaryData::residualVoltageRms, &DemandData::voltageResidualRms);
    addExtractors(ds, &OneSecondSummaryData::residualVoltageFundamental, &DemandData::voltageResidualFundamental);

    return ds;
}

DataSource DataSourceFactory::createCurrentRmsSource()
{
    DataSource ds;
    ds.name = "";
    ds.rowLabels = {"A", "B", "C", "Average"};
    addPhaseGroupExtractors(ds, &OneSecondSummaryData::totalCurrentRms, &DemandData::totalCurrentRms);

    // Average
    addAverageExtractors<double>(
        ds,
        [](const OneSecondSummaryData& s) { return (s.totalCurrentRms.a + s.totalCurrentRms.b + s.totalCurrentRms.c) / 3.0; },
        &DemandData::averageTotalCurrentRms);

    return ds;
}

DataSource DataSourceFactory::createCurrentFundamentalSource()
{
    DataSource ds;
    ds.name = "";
    ds.rowLabels = {"AB", "BC", "CA", "Average"};

    auto extractRms = [](const HarmonicAnalysisResult &h) { return h.rms; };

    addPhaseGroupExtractors(ds, &OneSecondSummaryData::fundamentalCurrent, &DemandData::fundamentalCurrentRMS, extractRms);
    addAverageExtractors<double>(
        ds,
        [](const OneSecondSummaryData& s) { return (s.fundamentalCurrent.a.rms + s.fundamentalCurrent.b.rms + s.fundamentalCurrent.c.rms) / 3.0; },
        &DemandData::averageFundamentalCurrentRms
        );

    return ds;
}

DataSource DataSourceFactory::createCurrentThdSource()
{
    DataSource ds;
    ds.name = "";
    ds.rowLabels = {"A", "B", "C"};

    addPhaseGroupMaxOnlyExtractors(ds, &OneSecondSummaryData::currentThd, &DemandData::currentThd);

    return ds;
}

DataSource DataSourceFactory::createCurrentResidualSource()
{
    DataSource ds;
    ds.name = "";
    ds.rowLabels = {"RMS", "Fund."};

    addExtractors(ds, &OneSecondSummaryData::residualCurrentRms, &DemandData::currentResidualRms);
    addExtractors(ds, &OneSecondSummaryData::residualCurrentFundamental, &DemandData::currentResidualFundamental);

    return ds;
}

DataSource DataSourceFactory::createPowerActiveSource()
{
    DataSource ds;
    ds.name = "";
    ds.rowLabels = {"A", "B", "C", "Total"};

    addPhaseGroupExtractors(ds, &OneSecondSummaryData::activePower, &DemandData::activePower);
    addExtractors(ds, &OneSecondSummaryData::totalActivePower, &DemandData::totalActivePower);

    return ds;
}

DataSource DataSourceFactory::createPowerReactiveSource()
{
    DataSource ds;
    ds.name = "";
    ds.rowLabels = {"A", "B", "C", "Total"};

    addPhaseGroupExtractors(ds, &OneSecondSummaryData::reactivePower, &DemandData::reactivePower);
    addExtractors(ds, &OneSecondSummaryData::totalReactivePower, &DemandData::totalReactivePower);

    return ds;
}

DataSource DataSourceFactory::createPowerApparentSource()
{
    DataSource ds;
    ds.name = "";
    ds.rowLabels = {"A", "B", "C", "Total"};

    addPhaseGroupExtractors(ds, &OneSecondSummaryData::apparentPower, &DemandData::apparentPower);
    addExtractors(ds, &OneSecondSummaryData::totalApparentPower, &DemandData::totalApparentPower);

    return ds;
}

DataSource DataSourceFactory::createPowerFactorSource()
{
    DataSource ds;
    ds.name = "";
    ds.rowLabels = {"A", "B", "C", "Total"};

    addPhaseGroupExtractors(ds, &OneSecondSummaryData::powerFactor, &DemandData::powerFactor);
    addExtractors(ds, &OneSecondSummaryData::totalPowerFactor, &DemandData::totalPowerFactor);

    return ds;
}

DataSource DataSourceFactory::createPowerEnergySource()
{
    DataSource ds;
    ds.name = "";
    ds.rowLabels = {"Net"};

    ds.extractors.push_back([](const OneSecondSummaryData& d) { return d.totalEnergyWh / 1e3; } );

    return ds;
}

DataSource DataSourceFactory::createVoltageSymmetricalLLSource()
{
    DataSource ds;
    ds.name = "L-L";
    ds.rowLabels = {"Positive-\nSequence", "Negative-\nSequence"};

    addSymmetricalGroupExtractors_ll(ds,
                                  &OneSecondSummaryData::voltageSymmetricalComponents_ll,
                                  &DemandData::voltageSymmetricalComponents_ll);

    return ds;
}

DataSource DataSourceFactory::createVoltageSymmetricalLNSource()
{
    DataSource ds;
    ds.name = "L-N";
    ds.rowLabels = {"Positive-\nSequence", "Negative-\nSequence", "Zero-\nSequence"};

    addSymmetricalGroupExtractors(ds,
                                  &OneSecondSummaryData::voltageSymmetricalComponents,
                                  &DemandData::voltageSymmetricalComponents);

    return ds;
}

DataSource DataSourceFactory::createVoltageUnbalanceSource()
{
    DataSource ds;
    ds.name = "";
    ds.rowLabels = {"NEMA", "NEMA", "Negative-\nSequence", "Zero-\nSequence"};

    addMaxOnlyExtractors(ds, &OneSecondSummaryData::nemaVoltageUnbalance_ll, &DemandData::nemaVoltageUnbalance_ll);
    addMaxOnlyExtractors(ds, &OneSecondSummaryData::nemaVoltageUnbalance, &DemandData::nemaVoltageUnbalance);
    addMaxOnlyExtractors(ds, &OneSecondSummaryData::voltageU2Unbalance, &DemandData::voltageU2Unbalance);
    addMaxOnlyExtractors(ds, &OneSecondSummaryData::voltageU0Unbalance, &DemandData::voltageU0Unbalance);

    return ds;
}


DataSource DataSourceFactory::createCurrentSymmetricalSource()
{
    DataSource ds;
    ds.name = "";
    ds.rowLabels = {"Positive-\nSequence", "Negative-\nSequence", "Zero-\nSequence"};

    addSymmetricalGroupExtractors(ds,
                                  &OneSecondSummaryData::currentSymmetricalComponents,
                                  &DemandData::currentSymmetricalComponents);

    return ds;
}

DataSource DataSourceFactory::createCurrentUnbalanceSource()
{
    DataSource ds;
    ds.name = "";
    ds.rowLabels = {"NEMA", "Negative-\nSequence", "Zero-\nSequence"};

    addMaxOnlyExtractors(ds, &OneSecondSummaryData::nemaCurrentUnbalance, &DemandData::nemaCurrentUnbalance);
    addMaxOnlyExtractors(ds, &OneSecondSummaryData::currentU2Unbalance, &DemandData::currentU2Unbalance);
    addMaxOnlyExtractors(ds, &OneSecondSummaryData::currentU0Unbalance, &DemandData::currentU0Unbalance);

    return ds;
}
