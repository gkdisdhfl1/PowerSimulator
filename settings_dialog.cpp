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

    ui->maxSizeSpinBox->setRange(config::Simulation::MinDataSize, config::Simulation::MaxDataSize);

    ui->graphWidthSpinBox->setSuffix(" s");
    ui->graphWidthSpinBox->setRange(config::GraphWidth::Min, config::GraphWidth::Max);
    ui->graphWidthSpinBox->setValue(config::GraphWidth::Default);

}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::setInitialValues(int maxSize, double graphWidth)
{
    ui->maxSizeSpinBox->setValue(maxSize);
    ui->graphWidthSpinBox->setValue(graphWidth);
}

// --- getter 함수들 ---
int SettingsDialog::getMaxSize() const { return ui->maxSizeSpinBox->value();}
double SettingsDialog::getGraphWidth() const { return ui->graphWidthSpinBox->value();}


std::expected<void, SettingsDialog::ValidationError> SettingsDialog::validateInput() const
{
    // config.h의 값들을 사용하여 유효성 검사
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
