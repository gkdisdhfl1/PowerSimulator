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

    ui->maxSizeSpinBox->setRange(config::MinDataSize, config::MaxDataSize);

    ui->graphWidthSpinBox->setSuffix(" s");
    ui->graphWidthSpinBox->setRange(config::MinGraphWidthSec, config::MaxGraphWidthSec);
    ui->graphWidthSpinBox->setValue(config::DefaultGraphWidthSec);

    ui->intervalSpinBox->setRange(config::MinIntervalSec, 60.0); // 최대 1분
    ui->intervalSpinBox->setSingleStep(0.1);
    ui->intervalSpinBox->setValue(config::DefaultIntervalMs / 1000.0);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::setInitialValues(double interval, int maxSize, double graphWidth)
{
    ui->intervalSpinBox->setValue(interval);
    ui->maxSizeSpinBox->setValue(maxSize);
    ui->graphWidthSpinBox->setValue(graphWidth);
}

void SettingsDialog::accept()
{
    // ui에서 값을 읽어옴
    double interval = ui->intervalSpinBox->value();
    int size = ui->maxSizeSpinBox->value();
    double graphWidth = ui->graphWidthSpinBox->value();

    // 유효성 검사
    QString errorMessage;
    if (interval < config::MinIntervalSec) {
        errorMessage += QString("간격은 %1초 이상이어야 합니다.\n").arg(config::MinIntervalSec);
    }
    if (size < config::MinDataSize)
    {
        errorMessage += QString("최대 데이터 크기는 %1 이상이어야 합니다.\n").arg(config::MinDataSize);
    }
    if (graphWidth < config::MinGraphWidthSec)
    {
        errorMessage += QString("그래프 폭은 %1초 이상이어야 합니다.\n").arg(config::MinGraphWidthSec);
    }

    if(!errorMessage.isEmpty()) {
        QMessageBox::warning(this, "입력 오류", errorMessage);
        return;
    }

    // 읽어온 값과 함께 시그널 발생
    emit settingsApplied(interval, size, graphWidth);

    // 부모 클래스의 accept()를 호출하여 dialog를 닫고 dialog.exec()가 QDialog::Accepted를 반환하도록 함
    QDialog::accept();
}
