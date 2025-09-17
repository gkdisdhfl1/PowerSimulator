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
    void settingsApplied(FrequencyTracker::PidCoefficients& fllCoeffs, FrequencyTracker::PidCoefficients& zcCoeffs);

private:
    void setupUi();
    void accept() override;

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
