#ifndef TRIGONOMETRIC_TABLE_H
#define TRIGONOMETRIC_TABLE_H


#include <vector>
class TrigonometricTable
{
public:
    TrigonometricTable(int N, int max_k);

    double getCos(int k, int n) const;
    double getSin(int k, int n) const;
    int getN() const;

private:
    int m_N;
    // 2차원 배열 대신 1차원 벡터로 메모리를 연속적으로 할당
    std::vector<double> m_cosTable;
    std::vector<double> m_sinTable;
};

#endif // TRIGONOMETRIC_TABLE_H
