#ifndef DATASOURCE_TYPES_H
#define DATASOURCE_TYPES_H

#include "demand_data.h"
#include "measured_data.h"
#include "min_max_tracker.h"

#include <functional>

using Extractor = std::function<double(const OneSecondSummaryData&)>;
using MinMaxExtractor = std::function<ValueWithTimestamp<double>(const DemandData&)>;

struct DataSource {
    QString name; // 버튼 이름
    QStringList rowLabels;
    std::vector<Extractor> extractors;

    // Min/Max용 Extractor
    std::vector<MinMaxExtractor> maxExtractors;
    std::vector<MinMaxExtractor> minExtractors;
};

#endif // DATASOURCE_TYPES_H
