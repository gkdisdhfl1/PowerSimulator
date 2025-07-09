#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"

#include <QMessageBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
    , m_graphWidthSpinBox(new QDoubleSpinBox(this))
{
    ui->setupUi(this);

    ui->maxSizeSpinBox->setRange(1, 10000);

    // '그래프 폭' 설정을 위한 위젯 생성 및 설정
    QLabel *graphWidthLabel = new QLabel("그래프 폭", this);
    QFont font = graphWidthLabel->font();
    font.setPointSize(12);
    graphWidthLabel->setFont(font);

    m_graphWidthSpinBox->setSuffix(" s");
    m_graphWidthSpinBox->setRange(1.0, 300.0); // 1초 ~ 5분
    m_graphWidthSpinBox->setValue(10.0); // 기본값 10초

    // 새 위젯들을 담을 레이아웃 생성
    QHBoxLayout *graphWidthLayout = new QHBoxLayout();
    graphWidthLayout->addWidget(graphWidthLabel);
    graphWidthLayout->addWidget(m_graphWidthSpinBox);

    // .ui 파일의 QVBoxLayout의 이름은 "verticalLayout"
    // 버튼들이 포함된 레이아웃(horizontalLayout_2) 앞에 새로운 레이아웃을 삽입
    int buttonLayoutIndex = ui->verticalLayout->count() - 1;
    ui->verticalLayout->insertLayout(buttonLayoutIndex, graphWidthLayout);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::setInitialValues(double interval, int maxSize, double graphWidth)
{
    ui->intervalSpinBox->setValue(interval);
    ui->maxSizeSpinBox->setValue(maxSize);
    m_graphWidthSpinBox->setValue(graphWidth);
}

void SettingsDialog::accept()
{
    // ui에서 값을 읽어옴
    double interval = ui->intervalSpinBox->value();
    int size = ui->maxSizeSpinBox->value();
    double graphWidth = m_graphWidthSpinBox->value();

    // 유효성 검사
    QString errorMessage;
    if (interval < 0.1) {
        errorMessage += "간격은 0.1초 이상이어야 합니다.\n";
    }
    if (size < 1)
    {
        errorMessage += "최대 데이터 크기는 1 이상이어야 합니다.\n";
    }
    if (graphWidth < 1.0)
    {
        errorMessage += "그래프 폭은 1초 이상이어야 합니다.\n";
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
