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
    ui->samplingCyclesDoubleSpinBox->setValue(10);
    ui->samplesPerCycleSpinBox->setRange(1, 1000);
    ui->samplesPerCycleSpinBox->setValue(10);

    ui->maxSizeSpinBox->setRange(config::Simulation::MinDataSize, config::Simulation::MaxDataSize);

    ui->graphWidthSpinBox->setSuffix(" s");
    ui->graphWidthSpinBox->setRange(config::GraphWidthSec::Min, config::GraphWidthSec::Max);
    ui->graphWidthSpinBox->setValue(config::GraphWidthSec::Default);

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

void SettingsDialog::accept()
{
    // ui에서 값을 읽어옴
    double samplingCycles = ui->samplingCyclesDoubleSpinBox->value();
    int samplesPerCycle = ui->samplesPerCycleSpinBox->value();
    int size = ui->maxSizeSpinBox->value();
    double graphWidth = ui->graphWidthSpinBox->value();

    // 초당 총 샘ㅍ르링 횟수 계산(시간 간격)
    int totalSamplesPerSecond = samplingCycles * samplesPerCycle;


    // 유효성 검사
    if(totalSamplesPerSecond <= 0) {
        QMessageBox::warning(this, "입력 오류", "초당 샘플링 횟수는 0보다 커야 합니다.");
        return;
    }
    double intervalnSeconds = 1.0 / static_cast<double>(totalSamplesPerSecond);
    if(intervalnSeconds < 0.001) { // 1ms 이하 짧은 간격 방지
        QMessageBox::warning(this, "입력 오류", "샘플링 간격이 너무 짧습니다. (1ms 이상 필요)");
        return;
    }

    QString errorMessage;
    if (size < config::Simulation::MinDataSize)
    {
        errorMessage += QString("최대 데이터 크기는 %1 이상이어야 합니다.\n").arg(config::Simulation::MinDataSize);
    }
    if (graphWidth < config::GraphWidthSec::Min)
    {
        errorMessage += QString("그래프 폭은 %1초 이상이어야 합니다.\n").arg(config::GraphWidthSec::Min);
    }

    if(!errorMessage.isEmpty()) {
        QMessageBox::warning(this, "입력 오류", errorMessage);
        return;
    }

    // 읽어온 값과 함께 시그널 발생
    emit settingsApplied(intervalnSeconds, size, graphWidth);

    // 부모 클래스의 accept()를 호출하여 dialog를 닫고 dialog.exec()가 QDialog::Accepted를 반환하도록 함
    QDialog::accept();
}
