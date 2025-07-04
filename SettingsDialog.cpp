#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"

#include <QMessageBox>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    ui->maxSizeSpinBox->setRange(1, 10000);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::setInitialValues(double interval, int maxSize)
{
    ui->intervalSpinBox->setValue(interval);
    ui->maxSizeSpinBox->setValue(maxSize);
}

void SettingsDialog::accept()
{
    // ui에서 값을 읽어옴
    double interval = ui->intervalSpinBox->value();
    int size = ui->maxSizeSpinBox->value();

    // 유효성 검사
    if(!(interval >= 0.1) && (size >= 1)) {
        QMessageBox::warning(this, "경고", "잘못된 입력입니다.");
        return;
    }

    // 읽어온 값과 함께 시그널 발생
    emit settingsApplied(interval, size);

    // 부모 클래스의 accept()를 호출하여 dialog를 닫고 dialog.exec()가 QDialog::Accepted를 반환하도록 함
    QDialog::accept();
}
