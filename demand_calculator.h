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
    template <typename T>
    void bindPhaseGroup(GenericPhaseData<MinMaxTracker<T>> DemandData::* targetGroup, const PhaseData OneSecondSummaryData::*sourceGroup) {
        m_mappings.push_back([targetGroup, sourceGroup](DemandData& d, const OneSecondSummaryData& s, const QDateTime& t) {
            (d.*targetGroup).a.update((s.*sourceGroup).a, t);
            (d.*targetGroup).b.update((s.*sourceGroup).b, t);
            (d.*targetGroup).c.update((s.*sourceGroup).c, t);
        });
    }

    // 선간 전압을 위한 바인더
    template <typename T>
    void bindLinetoLineGroup(GenericLinetoLineData<MinMaxTracker<T>> DemandData::* targetGroup, const LineToLineData OneSecondSummaryData::* sourceGroup) {
        m_mappings.push_back([targetGroup, sourceGroup](DemandData& d, const OneSecondSummaryData& s, const QDateTime& t) {
            (d.*targetGroup).ab.update((s.*sourceGroup).ab, t);
            (d.*targetGroup).bc.update((s.*sourceGroup).bc, t);
            (d.*targetGroup).ca.update((s.*sourceGroup).ca, t);
        });
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

    // 선간 전압을 위한 바인더
    template <typename T>
    void bindLinetoLineGroupMaxOnly(GenericLinetoLineData<MaxTracker<T>> DemandData::* targetGroup, const LineToLineData OneSecondSummaryData::* sourceGroup) {
        m_mappings.push_back([targetGroup, sourceGroup](DemandData& d, const OneSecondSummaryData& s, const QDateTime& t) {
            (d.*targetGroup).ab.update((s.*sourceGroup).ab, t);
            (d.*targetGroup).bc.update((s.*sourceGroup).bc, t);
            (d.*targetGroup).ca.update((s.*sourceGroup).ca, t);
        });
    }
};

#endif // DEMAND_CALCULATOR_H
