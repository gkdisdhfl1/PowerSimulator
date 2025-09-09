#ifndef BASE_GRAPH_WINDOW_H
#define BASE_GRAPH_WINDOW_H

#include "config.h"
#include <QWidget>
#include <deque>

// 전방 선언
class QChart;
class QValueAxis;
class CustomChartView;
class SimulationEngine;

using utils::FpSeconds;

class BaseGraphWindow : public QWidget
{
    Q_OBJECT
public:
    explicit BaseGraphWindow(SimulationEngine *engine, QWidget *parent = nullptr);
    virtual ~BaseGraphWindow() = default;

public slots:
    void toggleAutoScroll(bool enabled);

protected:
    using Nanoseconds = std::chrono::nanoseconds;

    void setupBaseChart();
    virtual void setupSeries() = 0; // 순수 가상 함수

    // 멤버 변수들을 protected로 이동하여 자식 클래스에서 접근 가능하도록 함
    std::unique_ptr<QChart> m_chart;
    QValueAxis *m_axisX;
    CustomChartView *m_chartView;
    SimulationEngine* m_engine;
    bool m_isAutoScrollEnabled;

    // X축의 현재 보이는 범위를 계산하는 헬퍼 함수
    template<typename Container>
    std::pair<double, double> getVisibleXRange(const Container& data);

    template<typename T>
    auto getVisibleRangeIterators(const std::deque<T>& data, Nanoseconds minTime, Nanoseconds maxTime) const {
        // 이진 탐색으로 시작점 찾기
        auto first = std::lower_bound(data.begin(), data.end(), minTime,
                                      [](const T& point, Nanoseconds time){
            return point.timestamp < time;});

        // 이진 탐색으로 끝점 찾기
        auto last = std::upper_bound(first, data.end(), maxTime,
                                     [](Nanoseconds time, const T& point) {
            return time < point.timestamp;
        });

        return std::make_pair(first, last);
    }

    template<typename Iterator, typename Point = typename std::iterator_traits<Iterator>::value_type>
    auto downsampleLTTB(Iterator first, Iterator last, int threshold, const std::vector<std::function<double(const Point&)>>& value_extractors) const
    {
        std::vector<Point> sampled_data;
        const int dataSize = std::distance(first, last);

        if(threshold >= dataSize || threshold <= 2) {
            sampled_data.assign(first, last);
            return sampled_data;
        }

        sampled_data.reserve(threshold);

        // 1. 첫 번째 점은 항상 선택
        sampled_data.push_back(*first);

        const double bucket_size = static_cast<double>(dataSize - 2) / (threshold - 2);

        for(int i{0}; i < threshold - 2; ++i) {
            // 2. 현재 버킷의 범위 계산
            const int bucket_start_index = static_cast<int>(std::floor((i) * bucket_size)) + 1;
            const int bucket_end_index = std::min(dataSize - 1, static_cast<int>(std::floor((i + 1) * bucket_size)) + 1);

            // 3. 다음 버킷의 평균점 계산
            const int next_bucket_start_index = bucket_end_index;
            const int next_bucket_end_index = std::min(dataSize, static_cast<int>(std::floor((i + 2) * bucket_size)) + 1);
            const int next_bucket_size = next_bucket_end_index - next_bucket_start_index;

            double avg_x = 0;
            std::vector<double> avg_ys(value_extractors.size(), 0.0);

            if(next_bucket_size > 0) {
                for(int j = next_bucket_start_index; j < next_bucket_end_index; ++j) {
                    avg_x += FpSeconds((*(first + j)).timestamp).count();
                    for(size_t k = 0; k < value_extractors.size(); ++k) {
                        avg_ys[k] += value_extractors[k](*(first + j));
                    }
                }
                avg_x /= next_bucket_size;
                for(size_t k = 0; k < value_extractors.size(); ++k) {
                    avg_ys[k] /= next_bucket_size;
                }
            }
            // 4. 현재 버킷 내에서 가장 중요도가 높은 점 찾기
            double max_effective_area = -1.0;
            auto point_to_add = first + bucket_start_index;

            const auto& prev_selected_point = sampled_data.back();
            const double prev_x = FpSeconds(prev_selected_point.timestamp).count();

            for(int j = bucket_start_index; j < bucket_end_index; ++j) {
                const auto& current_point = *(first + j);
                const double curr_x = FpSeconds(current_point.timestamp).count();

                double max_area_for_this_point = 0.0;
                // 모든 계열에 대해 삼각형 넓이를 계산하고, 그 중 최대값을 이 점의 중요도로 삼음
                for(size_t k = 0; k < value_extractors.size(); ++k) {
                    const double prev_y = value_extractors[k](prev_selected_point);
                    const double curr_y = value_extractors[k](current_point);
                    const double avg_y = avg_ys[k];
                    double area = std::abs(prev_x * (curr_y - avg_y) + curr_x * (avg_y - prev_y) + avg_x * (prev_y - curr_y)) / 2.0;

                    if(area > max_area_for_this_point) {
                        max_area_for_this_point = area;
                    }
                }

                if(max_area_for_this_point > max_effective_area) {
                    max_effective_area = max_area_for_this_point;
                    point_to_add = first + j;
                }
            }
            // 5. 가장 중요한 점을 결과에 추가
            sampled_data.push_back(*point_to_add);
        }

        // 6. 마지막 점은 항상 선택
        sampled_data.push_back(*(last - 1));

        return sampled_data;
    }
};

#endif // BASE_GRAPH_WINDOW_H
