#include "a3700n_datasource_factory.h"

// ==================================
// Private Static 헬퍼 함수들
// ==================================

template <typename SourceType, typename DemandType>
void DataSourceFactory::addExtractors(DataSource& dataSource,
                                  SourceType OneSecondSummaryData::* source,
                                  MinMaxTracker<DemandType> DemandData::* demand)
{
    // 기본 Extractor
    dataSource.extractors.push_back([source](const OneSecondSummaryData& s) { return s.*source; });

    // Max Extractor
    dataSource.maxExtractors.push_back([demand](const DemandData& d) { return (d.*demand).max; });

    // Min Extractor
    dataSource.minExtractors.push_back([demand](const DemandData& d) { return (d.*demand).min; });
}

template <typename SourceType, typename DemandType>
void DataSourceFactory::addMaxOnlyExtractors(DataSource& dataSource,
                                      SourceType OneSecondSummaryData::* source,
                                      MinMaxTracker<DemandType> DemandData::* demand)
{
    // 기본 Extractor
    dataSource.extractors.push_back([source](const OneSecondSummaryData& s) { return s.*source; });

    // Max Extractor
    dataSource.maxExtractors.push_back([demand](const DemandData& d) { return (d.*demand).max; });

}

template <typename T>
void DataSourceFactory::addPhaseGroupExtractors(
    DataSource& dataSource,
    const PhaseData OneSecondSummaryData::* sourceGroup,
    const GenericPhaseData<MinMaxTracker<T>> DemandData::* demandGroup)
{
    // A
    dataSource.extractors.push_back([sourceGroup](const OneSecondSummaryData& s) { return (s.*sourceGroup).a; });
    dataSource.maxExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).a.max; });
    dataSource.minExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).a.min; });

    // B
    dataSource.extractors.push_back([sourceGroup](const OneSecondSummaryData& s) { return (s.*sourceGroup).b; });
    dataSource.maxExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).b.max; });
    dataSource.minExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).b.min; });

    // C
    dataSource.extractors.push_back([sourceGroup](const OneSecondSummaryData& s) { return (s.*sourceGroup).c; });
    dataSource.maxExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).c.max; });
    dataSource.minExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).c.min; });
}

template <typename T>
void DataSourceFactory::addPhaseGroupMaxOnlyExtractors(
    DataSource& dataSource,
    const PhaseData OneSecondSummaryData::* sourceGroup,
    const GenericPhaseData<MaxTracker<T>> DemandData::* demandGroup)
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

template <typename T>
void DataSourceFactory::addLinetoLineGroupExtractors(
    DataSource& dataSource,
    const LineToLineData OneSecondSummaryData::* sourceGroup,
    const GenericLinetoLineData<MinMaxTracker<T>> DemandData::* demandGroup)
{
    dataSource.extractors.push_back([sourceGroup](const OneSecondSummaryData& s) { return (s.*sourceGroup).ab;});
    dataSource.maxExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).ab.max; });
    dataSource.minExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).ab.min; });

    dataSource.extractors.push_back([sourceGroup](const OneSecondSummaryData& s) { return (s.*sourceGroup).bc;});
    dataSource.maxExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).bc.max; });
    dataSource.minExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).bc.min; });

    dataSource.extractors.push_back([sourceGroup](const OneSecondSummaryData& s) { return (s.*sourceGroup).ca;});
    dataSource.maxExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).ca.max; });
    dataSource.minExtractors.push_back([demandGroup](const DemandData& d) { return (d.*demandGroup).ca.min; });
}

template <typename T>
void DataSourceFactory::addLinetoLineGroupMaxOnlyExtractors(
    DataSource& dataSource,
    const LineToLineData OneSecondSummaryData::* sourceGroup,
    const GenericLinetoLineData<MaxTracker<T>> DemandData::* demandGroup)
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

    // --- 기본 Extractor ---
    llSource.extractors.push_back([](const OneSecondSummaryData& s) { return s.fundamentalVoltage_ll[0].rms; });
    llSource.extractors.push_back([](const OneSecondSummaryData& s) { return s.fundamentalVoltage_ll[1].rms; });
    llSource.extractors.push_back([](const OneSecondSummaryData& s) { return s.fundamentalVoltage_ll[2].rms; });
    llSource.extractors.push_back([](const OneSecondSummaryData& s) {
        return (s.fundamentalVoltage_ll[0].rms + s.fundamentalVoltage_ll[1].rms + s.fundamentalVoltage_ll[2].rms) / 3.0;
    });

    // --- Max Extractor ---
    llSource.maxExtractors.push_back([](const DemandData& d) { return d.fundamentalVoltageRMS_ll.ab.max; });
    llSource.maxExtractors.push_back([](const DemandData& d) { return d.fundamentalVoltageRMS_ll.bc.max; });
    llSource.maxExtractors.push_back([](const DemandData& d) { return d.fundamentalVoltageRMS_ll.ca.max; });
    llSource.maxExtractors.push_back([](const DemandData& d) { return d.averageFundamentalVoltageRms_ll.max; });

    // --- Mix Extractor ---
    llSource.minExtractors.push_back([](const DemandData& d) { return d.fundamentalVoltageRMS_ll.ab.min; });
    llSource.minExtractors.push_back([](const DemandData& d) { return d.fundamentalVoltageRMS_ll.bc.min; });
    llSource.minExtractors.push_back([](const DemandData& d) { return d.fundamentalVoltageRMS_ll.ca.min; });
    llSource.minExtractors.push_back([](const DemandData& d) { return d.averageFundamentalVoltageRms_ll.min; });

    return llSource;
}

DataSource DataSourceFactory::createVoltageFundamentalLNSource()
{
    DataSource lnSource;
    lnSource.name = "L-N";
    lnSource.rowLabels = {"A", "B", "C", "Average"};

    // --- 기본 Extractor ---
    lnSource.extractors.push_back([](const OneSecondSummaryData& s) { return s.fundamentalVoltage[0].rms; });
    lnSource.extractors.push_back([](const OneSecondSummaryData& s) { return s.fundamentalVoltage[1].rms; });
    lnSource.extractors.push_back([](const OneSecondSummaryData& s) { return s.fundamentalVoltage[2].rms; });
    lnSource.extractors.push_back([](const OneSecondSummaryData& s) {
        return (s.fundamentalVoltage[0].rms + s.fundamentalVoltage[1].rms + s.fundamentalVoltage[2].rms) / 3.0;
    });

    // --- Max Extractor ---
    lnSource.maxExtractors.push_back([](const DemandData& d) { return d.fundamentalVoltageRMS.a.max; });
    lnSource.maxExtractors.push_back([](const DemandData& d) { return d.fundamentalVoltageRMS.b.max; });
    lnSource.maxExtractors.push_back([](const DemandData& d) { return d.fundamentalVoltageRMS.c.max; });
    lnSource.maxExtractors.push_back([](const DemandData& d) { return d.averageFundamentalVoltageRms.max; });

    // --- Mix Extractor ---
    lnSource.minExtractors.push_back([](const DemandData& d) { return d.fundamentalVoltageRMS.a.min; });
    lnSource.minExtractors.push_back([](const DemandData& d) { return d.fundamentalVoltageRMS.b.min; });
    lnSource.minExtractors.push_back([](const DemandData& d) { return d.fundamentalVoltageRMS.c.min; });
    lnSource.minExtractors.push_back([](const DemandData& d) { return d.averageFundamentalVoltageRms.min; });

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

    // --- 기본 Extractor ---
    ds.extractors.push_back([](const OneSecondSummaryData& s) { return s.fundamentalCurrent[0].rms; });
    ds.extractors.push_back([](const OneSecondSummaryData& s) { return s.fundamentalCurrent[1].rms; });
    ds.extractors.push_back([](const OneSecondSummaryData& s) { return s.fundamentalCurrent[2].rms; });
    ds.extractors.push_back([](const OneSecondSummaryData& s) {
        return (s.fundamentalCurrent[0].rms + s.fundamentalCurrent[1].rms + s.fundamentalCurrent[2].rms) / 3.0;
    });

    // --- Max Extractor ---
    ds.maxExtractors.push_back([](const DemandData& d) { return d.fundamentalCurrentRMS.a.max; });
    ds.maxExtractors.push_back([](const DemandData& d) { return d.fundamentalCurrentRMS.b.max; });
    ds.maxExtractors.push_back([](const DemandData& d) { return d.fundamentalCurrentRMS.c.max; });
    ds.maxExtractors.push_back([](const DemandData& d) { return d.averageFundamentalCurrentRms.max; });

    // --- Mix Extractor ---
    ds.minExtractors.push_back([](const DemandData& d) { return d.fundamentalCurrentRMS.a.min; });
    ds.minExtractors.push_back([](const DemandData& d) { return d.fundamentalCurrentRMS.b.min; });
    ds.minExtractors.push_back([](const DemandData& d) { return d.fundamentalCurrentRMS.c.min; });
    ds.minExtractors.push_back([](const DemandData& d) { return d.averageFundamentalCurrentRms.min; });

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

    // Default Extractor
    ds.extractors.push_back([](const OneSecondSummaryData& s) { return s.voltageSymmetricalComponents_ll.positive.magnitude; });
    ds.extractors.push_back([](const OneSecondSummaryData& s) { return s.voltageSymmetricalComponents_ll.negative.magnitude; });

    // Max Extractor
    ds.maxExtractors.push_back([](const DemandData& d) { return d.voltageSymmetricalPositive_ll; });
    ds.maxExtractors.push_back([](const DemandData& d) { return d.voltageSymmetricalNegative_ll; });

    return ds;
}

DataSource DataSourceFactory::createVoltageSymmetricalLNSource()
{
    DataSource ds;
    ds.name = "L-N";
    ds.rowLabels = {"Positive-\nSequence", "Negative-\nSequence", "Zero-\nSequence"};

    // Default Extractor
    ds.extractors.push_back([](const OneSecondSummaryData& s) { return s.voltageSymmetricalComponents.positive.magnitude; });
    ds.extractors.push_back([](const OneSecondSummaryData& s) { return s.voltageSymmetricalComponents.negative.magnitude; });
    ds.extractors.push_back([](const OneSecondSummaryData& s) { return s.voltageSymmetricalComponents.zero.magnitude; });

    // Max Extractor
    ds.maxExtractors.push_back([](const DemandData& d) { return d.voltageSymmetricalPositive; });
    ds.maxExtractors.push_back([](const DemandData& d) { return d.voltageSymmetricalNegative; });
    ds.maxExtractors.push_back([](const DemandData& d) { return d.voltageSymmetricalZero; });

    return ds;
}

DataSource DataSourceFactory::createVoltageUnbalanceSource()
{
    DataSource ds;
    ds.name = "";
    ds.rowLabels = {"NEMA", "NEMA", "Negative-\nSequence", "Zero-\nSequence"};

    // Default Extractor
    ds.extractors.push_back([](const OneSecondSummaryData& s) { return s.nemaVoltageUnbalance_ll; });
    ds.extractors.push_back([](const OneSecondSummaryData& s) { return s.nemaVoltageUnbalance; });
    ds.extractors.push_back([](const OneSecondSummaryData& s) { return s.voltageU2Unbalance; });
    ds.extractors.push_back([](const OneSecondSummaryData& s) { return s.voltageU0Unbalance; });

    // Max Extractor
    ds.maxExtractors.push_back([](const DemandData& d) { return d.nemaVoltageUnbalance_ll; });
    ds.maxExtractors.push_back([](const DemandData& d) { return d.nemaVoltageUnbalance; });
    ds.maxExtractors.push_back([](const DemandData& d) { return d.voltageU2Unbalance; });
    ds.maxExtractors.push_back([](const DemandData& d) { return d.voltageU0Unbalance; });

    return ds;
}


DataSource DataSourceFactory::createCurrentSymmetricalSource()
{
    DataSource ds;
    ds.name = "";
    ds.rowLabels = {"Positive-\nSequence", "Negative-\nSequence", "Zero-\nSequence"};

    // Default Extractor
    ds.extractors.push_back([](const OneSecondSummaryData& s) { return s.currentSymmetricalComponents.positive.magnitude; });
    ds.extractors.push_back([](const OneSecondSummaryData& s) { return s.currentSymmetricalComponents.negative.magnitude; });
    ds.extractors.push_back([](const OneSecondSummaryData& s) { return s.currentSymmetricalComponents.zero.magnitude; });

    // Max Extractor
    ds.maxExtractors.push_back([](const DemandData& d) { return d.currentSymmetricalPositive; });
    ds.maxExtractors.push_back([](const DemandData& d) { return d.currentSymmetricalNegative; });
    ds.maxExtractors.push_back([](const DemandData& d) { return d.currentSymmetricalZero; });

    return ds;
}

DataSource DataSourceFactory::createCurrentUnbalanceSource()
{
    DataSource ds;
    ds.name = "";
    ds.rowLabels = {"NEMA", "Negative-\nSequence", "Zero-\nSequence"};

    ds.extractors.push_back([](const OneSecondSummaryData& s) { return s.nemaCurrentUnbalance; });
    ds.extractors.push_back([](const OneSecondSummaryData& s) { return s.currentU2Unbalance; });
    ds.extractors.push_back([](const OneSecondSummaryData& s) { return s.currentU0Unbalance; });

    // Max Extractor
    ds.maxExtractors.push_back([](const DemandData& d) { return d.nemaCurrentUnbalance; });
    ds.maxExtractors.push_back([](const DemandData& d) { return d.currentU2Unbalance; });
    ds.maxExtractors.push_back([](const DemandData& d) { return d.currentU0Unbalance; });

    return ds;
}
