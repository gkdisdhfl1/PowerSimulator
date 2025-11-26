#ifndef DEMAND_CALCULATOR_H
#define DEMAND_CALCULATOR_H

#include <QObject>
#include "demand_data.h"
#include "measured_data.h"

class DemandCalculator : public QObject
{
    Q_OBJECT
public:
    explicit DemandCalculator(QObject *parent = nullptr);

    // 외부에서 Max/Min 데이터에 접근하기 위한 getter
    const DemandData& getDemandData() const {
        return m_demandData;
    }

public slots:
    void processOneSecondData(const OneSecondSummaryData& summary);

signals:
    void demandDataUpdated(const DemandData& data);

private:
    void initializeMapping();

    DemandData m_demandData;

    // 업데이트 함수들을 담을 벡터
    using UpdaterFunc = std::function<void(DemandData&, const OneSecondSummaryData&, const QDateTime&)>;
    std::vector<UpdaterFunc> m_mappings;

    // -- 헬퍼 템플릿 함수들 ---

    // MinMaxTracker<T> 타입의 단일 멤버를 위한 바인더
    template <typename T>
    void bindMinMax(MinMaxTracker<T> DemandData::* target, const T OneSecondSummaryData::* source) {
        m_mappings.push_back([target, source](DemandData& d, const OneSecondSummaryData& s, const QDateTime& t) {
            (d.*target).update(s.*source, t);
        });
    }

    // MaxTracker<T> 타입의 단일 멤버를 위한 바인더
    template <typename T>
    void bindMaxOnly(MaxTracker<T> DemandData::* target, const T OneSecondSummaryData::* source) {
        m_mappings.push_back([target, source](DemandData& d, const OneSecondSummaryData& s, const QDateTime& t) {
            (d.*target).update(s.*source, t);
        });
    }

    // 3상 그룹을 위한 바인더
    template <typename TargetGroupMemberPtr, typename SourceGroupMemberPtr, typename ValueExtractor>
    void bindPhaseGroup(
        TargetGroupMemberPtr targetGroup,
        SourceGroupMemberPtr sourceGroup,
        ValueExtractor valueExtractor
    ) {
        m_mappings.push_back([targetGroup, sourceGroup, valueExtractor](DemandData& d, const OneSecondSummaryData& s, const QDateTime& t) {
            // 컴파일 타임에 .a, .b, .c 접근 가능 여부 체크됨
            auto& target = d.*targetGroup;
            const auto& source = s.*sourceGroup;

            target.a.update(valueExtractor(source.a), t);
            target.b.update(valueExtractor(source.b), t);
            target.c.update(valueExtractor(source.c), t);
        });
    }

    // ValueExtractor가 없는 기본 버전
    template <typename TargetGroupMemberPtr, typename SourceGroupMemberPtr>
    void bindPhaseGroup(
        TargetGroupMemberPtr targetGroup,
        SourceGroupMemberPtr sourceGroup
    ) {
        // 기본값: 그대로 전달
        bindPhaseGroup(targetGroup, sourceGroup, [](const auto& v) { return v; });
    }


    // 선간 전압을 위한 바인더
    template <typename TargetGroupMemberPtr, typename SourceGroupMemberPtr, typename ValueExtractor>
    void bindLinetoLineGroup(
        TargetGroupMemberPtr targetGroup,
        SourceGroupMemberPtr sourceGroup,
        ValueExtractor valueExtractor
    ) {
        m_mappings.push_back([targetGroup, sourceGroup, valueExtractor](DemandData& d, const OneSecondSummaryData& s, const QDateTime& t) {
            auto& target = d.*targetGroup;
            const auto& source = s.*sourceGroup;

            target.ab.update(valueExtractor(source.ab), t);
            target.bc.update(valueExtractor(source.bc), t);
            target.ca.update(valueExtractor(source.ca), t);
        });
    }
    template <typename TargetGroupMemberPtr, typename SourceGroupMemberPtr>
    void bindLinetoLineGroup(
        TargetGroupMemberPtr targetGroup,
        SourceGroupMemberPtr sourceGroup
    ) {
        bindLinetoLineGroup(targetGroup, sourceGroup, [](const auto& v) { return v; });
    }

    // 3상 Average 바인더
    template <typename TargetGroupMemberPtr, typename SourceGroupMemberPtr, typename ValueExtractor>
    void bindPhaseAverage(
        TargetGroupMemberPtr targetGroup,
        SourceGroupMemberPtr sourceGroup,
        ValueExtractor valueExtractor
    ) {
        m_mappings.push_back([=](DemandData& d, const OneSecondSummaryData& s, const QDateTime& t) {
            auto& target = d.*targetGroup;
            const auto& source = s.*sourceGroup;

            double sum = valueExtractor(source.a) + valueExtractor(source.b) + valueExtractor(source.c);
            target.update(sum / 3.0, t);
        });
    }
    template <typename TargetGroupMemberPtr, typename SourceGroupMemberPtr>
    void bindPhaseAverage(
        TargetGroupMemberPtr targetGroup,
        SourceGroupMemberPtr sourceGroup
    ) {
        bindPhaseAverage(targetGroup, sourceGroup, [](const auto& v) { return v; });
    }

    // 선간 Average 바인더
    template <typename TargetGroupMemberPtr, typename SourceGroupMemberPtr, typename ValueExtractor>
    void bindLinetoLineAverage(
        TargetGroupMemberPtr targetGroup,
        SourceGroupMemberPtr sourceGroup,
        ValueExtractor valueExtractor
        ) {
        m_mappings.push_back([=](DemandData& d, const OneSecondSummaryData& s, const QDateTime& t) {
            auto& target = d.*targetGroup;
            const auto& source = s.*sourceGroup;

            double sum = valueExtractor(source.ab) + valueExtractor(source.bc) + valueExtractor(source.ca);
            target.update(sum / 3.0, t);
        });
    }
    template <typename TargetGroupMemberPtr, typename SourceGroupMemberPtr>
    void bindLinetoLineAverage(
        TargetGroupMemberPtr targetGroup,
        SourceGroupMemberPtr sourceGroup
        ) {
        bindLinetoLineAverage(targetGroup, sourceGroup, [](const auto& v) { return v; });
    }


    // 3상 + MaxOnly 그룹을 위한 바인더
    template <typename T>
    void bindPhaseGroupMaxOnly(GenericPhaseData<MaxTracker<T>> DemandData::* targetGroup, const PhaseData OneSecondSummaryData::*sourceGroup) {
        m_mappings.push_back([targetGroup, sourceGroup](DemandData& d, const OneSecondSummaryData& s, const QDateTime& t) {
            (d.*targetGroup).a.update((s.*sourceGroup).a, t);
            (d.*targetGroup).b.update((s.*sourceGroup).b, t);
            (d.*targetGroup).c.update((s.*sourceGroup).c, t);
        });
    }

    // 선간 전압 + MaxOnly 그룹을 위한 바인더
    template <typename T>
    void bindLinetoLineGroupMaxOnly(GenericLinetoLineData<MaxTracker<T>> DemandData::* targetGroup, const LineToLineData OneSecondSummaryData::* sourceGroup) {
        m_mappings.push_back([targetGroup, sourceGroup](DemandData& d, const OneSecondSummaryData& s, const QDateTime& t) {
            (d.*targetGroup).ab.update((s.*sourceGroup).ab, t);
            (d.*targetGroup).bc.update((s.*sourceGroup).bc, t);
            (d.*targetGroup).ca.update((s.*sourceGroup).ca, t);
        });
    }

    // 3상 대칭 성분 그룹을 위한 바인더
    template <typename TargetSymmetricalGroupMemberPtr, typename SourceSymmetricalGroupMemberPtr>
    void bindPhaseSymmetricalGroup(
        TargetSymmetricalGroupMemberPtr targetGroup,
        SourceSymmetricalGroupMemberPtr sourceGroup)
    {
        m_mappings.push_back([=](DemandData& d, const OneSecondSummaryData& s, const QDateTime& t) {
            auto& target = d.*targetGroup;
            const auto& source = s.*sourceGroup;

            target.zero.update(source.zero.magnitude, t);
            target.positive.update(source.positive.magnitude, t);
            target.negative.update(source.negative.magnitude, t);
        });
    }

    // 선간 전압 대칭 성분 그룹을 위한 바인더
    template <typename TargetSymmetricalGroupMemberPtr, typename SourceSymmetricalGroupMemberPtr>
    void bindLineToLineSymmetricalGroup(
        TargetSymmetricalGroupMemberPtr targetGroup,
        SourceSymmetricalGroupMemberPtr sourceGroup)
    {
        m_mappings.push_back([=](DemandData& d, const OneSecondSummaryData& s, const QDateTime& t) {
            auto& target = d.*targetGroup;
            const auto& source = s.*sourceGroup;

            target.positive.update(source.positive.magnitude, t);
            target.negative.update(source.negative.magnitude, t);
        });
    }
};

#endif // DEMAND_CALCULATOR_H
