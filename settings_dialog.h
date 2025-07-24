#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <expected>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    enum class ValidationError {
        SamplingCyclesOutOfRange,
        SamplesPerCycleOutOfRange,
        MaxSizeOutOfRange,
        GraphWidthOutOfRange
    };

    // MainWindow에서 초기값을 설정해주기 위한 함수
    void setInitialValues(double samplingCycles, int samplesPerCycle, int maxSize, double graphWidth);

    // 결과를 가져갈 getter 함수들
    double getSamplingCycles() const;
    int getSamplesPerCycle() const;
    int getMaxSize() const;
    double getGraphWidth() const;

signals:
    void settingsApplied(double samplingCycles, int samplesPerCycle, int maxSize, double graphWidth);

protected:
    void accept() override;

private:
    Ui::SettingsDialog *ui;

    std::expected<void, ValidationError> validateInput() const;
    QString getErrorMessage(ValidationError error) const;
};

#endif // SETTINGS_DIALOG_H
