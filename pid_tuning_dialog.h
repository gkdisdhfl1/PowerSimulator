#ifndef PID_TUNING_DIALOG_H
#define PID_TUNING_DIALOG_H

#include <QDialog>
#include <QObject>
#include "frequency_tracker.h"

class ValueControlWidget;
class QGroupBox;
class QPushButton;

class PidTuningDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PidTuningDialog(QWidget *parent = nullptr);

    // 다이얼 로그를 열 때 현재 PID 값으로 초기화하는 함수
    void setInitialValues(const FrequencyTracker::PidCoefficients& fllCoeffs, const FrequencyTracker::PidCoefficients& zcCoeffs);

signals:
    // PID 계수가 변경될 때마다 발생하는 시그널
    void fllCoefficientsChanged(const FrequencyTracker::PidCoefficients& coeffs);
    void zcCoefficientsChanged(const FrequencyTracker::PidCoefficients& coeffs);

public slots:
    void onFllValuesChanged();
    void onZcValuesChanged();
    void applyChanges();

private:
    void setupUi();

    // FLL 그룹
    QGroupBox* m_fllGroup;
    ValueControlWidget* m_fllKpControl;
    ValueControlWidget* m_fllKiControl;
    ValueControlWidget* m_fllKdControl;

    // ZC 그룹
    QGroupBox* m_zcGroup;
    ValueControlWidget* m_zcKpControl;
    ValueControlWidget* m_zcKiControl;
    ValueControlWidget* m_zcKdControl;

    // 버튼
    QPushButton* m_applyButton;
    QPushButton* m_closeButton;
};

#endif // PID_TUNING_DIALOG_H
