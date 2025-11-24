#include "data_row_widget.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QDateTime>

DataRowWidget::DataRowWidget(const QString& name, const QString& unit, bool hasLine, QWidget* parent)
    : QWidget{parent}
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);// 라벨과 선 사이 간격

    // 1. 이름 - 값 - 단위를 담을 box
    auto rowLayout = new QHBoxLayout();

    m_nameLabel = new QLabel(name, this);
    m_nameLabel->setMinimumWidth(60); // 이름 영역 너비 고정
    m_nameLabel->setObjectName("nameLabel");

    m_timestampLabel = new QLabel("", this);
    m_timestampLabel->setObjectName("timestampLabel");
    m_timestampLabel->setAlignment(Qt::AlignCenter);
    m_timestampLabel->hide(); // 기본적으로 숨김

    m_valueLabel = new QLabel("0.000", this);
    m_valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_valueLabel->setObjectName("valueLabel");

    QLabel* unitLabel = new QLabel(unit, this);
    unitLabel->setMinimumWidth(20); // 단위 영역 너비 고정
    unitLabel->setObjectName("unitLabel");

    rowLayout->addWidget(m_nameLabel);
    rowLayout->addWidget(m_timestampLabel);
    rowLayout->addWidget(m_valueLabel, 1); // 값 라벨이 남은 공간 모두 차지
    rowLayout->addWidget(unitLabel);

    mainLayout->addLayout(rowLayout);

    // 2. 하단 라인
    if(hasLine) {
        QFrame* hLine = new QFrame(this);
        hLine->setFrameShape(QFrame::HLine);
        hLine->setFixedHeight(1);
        hLine->setObjectName("hLine");
        mainLayout->addWidget(hLine);
    }
}

void DataRowWidget::setValue(double value)
{
    setValue(value, QDateTime());
}

void DataRowWidget::setValue(double value, const QDateTime& timestamp)
{
    m_valueLabel->setText(QString::number(value, 'f', 3));
    if(timestamp.isValid()) {
        m_timestampLabel->setText(timestamp.toString("yyyy-MM-dd HH:mm:ss"));
        m_timestampLabel->show();
    } else {
        m_timestampLabel->hide();
    }
}

void DataRowWidget::setLabel(const QString& label)
{
    m_nameLabel->setText(label);
}
