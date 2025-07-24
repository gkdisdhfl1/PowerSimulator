#include "settings_dialog.h"
#include "config.h"
#include "ui_settings_dialog.h"

#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    ui->samplingCyclesDoubleSpinBox->setRange(1, 1000);
    ui->samplingCyclesDoubleSpinBox->setValue(config::Sampling::DefaultSamplingCycles);
    ui->samplesPerCycleSpinBox->setRange(1, 1000);
    ui->samplesPerCycleSpinBox->setValue(config::Sampling::DefaultSamplesPerCycle);

    ui->maxSizeSpinBox->setRange(config::Simulation::MinDataSize, config::Simulation::MaxDataSize);

    ui->graphWidthSpinBox->setSuffix(" s");
    ui->graphWidthSpinBox->setRange(config::GraphWidth::Min, config::GraphWidth::Max);
    ui->graphWidthSpinBox->setValue(config::GraphWidth::Default);

}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::setInitialValues(double samplingCycles, int samplesPerCycle, int maxSize, double graphWidth)
{
    ui->samplingCyclesDoubleSpinBox->setValue(samplingCycles);
    ui->samplesPerCycleSpinBox->setValue(samplesPerCycle);
    ui->maxSizeSpinBox->setValue(maxSize);
    ui->graphWidthSpinBox->setValue(graphWidth);
}

// --- getter 함수들 ---
double SettingsDialog::getSamplingCycles() const { return ui->samplingCyclesDoubleSpinBox->value();}
int SettingsDialog::getSamplesPerCycle() const { return ui->samplesPerCycleSpinBox->value();}
int SettingsDialog::getMaxSize() const { return ui->maxSizeSpinBox->value();}
double SettingsDialog::getGraphWidth() const { return ui->graphWidthSpinBox->value();}


std::expected<void, SettingsDialog::ValidationError> SettingsDialog::validateInput() const
{
    // config.h의 값들을 사용하여 유효성 검사
    const double samplingCycles = ui->samplingCyclesDoubleSpinBox->value();
    if(samplingCycles < config::Sampling::MinValue)
        return std::unexpected(ValidationError::SamplingCyclesOutOfRange);

    const double samplesPerCycle = ui->samplesPerCycleSpinBox->value();
    if(samplesPerCycle < config::Sampling::MinValue)
        return std::unexpected(ValidationError::SamplesPerCycleOutOfRange);

    const int maxSizeValue = ui->maxSizeSpinBox->value();
    if(maxSizeValue < config::Simulation::MinDataSize || maxSizeValue > config::Simulation::MaxDataSize)
        return std::unexpected(ValidationError::MaxSizeOutOfRange);

    const double graphWidthValue = ui->graphWidthSpinBox->value();
    if(graphWidthValue < config::GraphWidth::Min || graphWidthValue > config::GraphWidth::Max)
        return std::unexpected(ValidationError::GraphWidthOutOfRange);

    return {}; // 모든 검사 통과
}

QString SettingsDialog::getErrorMessage(ValidationError error) const
{
    std::string errorMessage;
    switch (error) {
    case ValidationError::SamplingCyclesOutOfRange:
        errorMessage = std::format("초 당 cycle은 {} 이상이어야 합니다.", config::Sampling::MinValue);
        break;
    case ValidationError::SamplesPerCycleOutOfRange:
        errorMessage = std::format("cycle 당 sample은 {} 이상이어야 합니다.", config::Sampling::MinValue);
        break;
    case ValidationError::MaxSizeOutOfRange:
        errorMessage = std::format("저장 크기는 {}와 {} 사이여야 합니다.", config::Simulation::MinDataSize, config::Simulation::MaxDataSize);
        break;
    case ValidationError::GraphWidthOutOfRange:
        errorMessage = std::format("그래프 폭은 {}와 {} 사이여야 합니다.", config::GraphWidth::Min, config::GraphWidth::Max);
        break;
    }
    return QString::fromStdString(errorMessage);
}

void SettingsDialog::accept()
{
    auto validationResult = validateInput();
    if(validationResult.has_value())
        // 유효성 검사 성공 시, 다이얼로그를 닫음
        QDialog::accept();
    else {
        // 실패 시, 오류 메세지 박스를 띄움
        const QString errorMessage = getErrorMessage(validationResult.error());
        QMessageBox::warning(this, "입력 값 오류", errorMessage);
    }

}
