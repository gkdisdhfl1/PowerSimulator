#include "trigonometric_table.h"
#include "config.h"
#include <qdebug>

TrigonometricTable::TrigonometricTable(int N, int max_k) : m_N(N)
{
    if(N <= 0 || max_k < 0) {
        return;
    }

    // k = 1부터 시작하지만 편의를 위해 0도 포함
    const int num_k = max_k + 1;
    m_cosTable.resize(num_k * N);
    m_sinTable.resize(num_k * N);

    for(int k = 0; k < num_k; ++k) {
        for(int n = 0; n < N; ++n) {
            const double angle = config::Math::TwoPi * k * n / N;
            m_cosTable[k * N + n] = std::cos(angle);
            m_sinTable[k * N + n] = std::sin(angle);
        }
    }

    qDebug() << "N: " << N;
    qDebug() << "테이블 생성됨";
}

double TrigonometricTable::getCos(int k, int n) const {
    // 간단한 경계 체크
    if(k * m_N + n >= m_cosTable.size())
        return 1.0;
    return m_cosTable[k * m_N + n];
}

double TrigonometricTable::getSin(int k, int n) const {
    // 간단한 경계 체크
    if(k * m_N + n >= m_sinTable.size())
        return 0.0;
    return m_sinTable[k * m_N + n];
}

int TrigonometricTable::getN() const
{
    return m_N;
}
