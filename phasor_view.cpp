#include "phasor_view.h"

#include <QCheckBox>
#include <QVBoxLayout>
#include <QPainter>

namespace {
    // 페이저를 그릴 때 사용하는 상수들
    constexpr double VOLTAGE_RADIUS = 100.0;
    constexpr double CURRENT_RADIUS = 80.0;
    constexpr int AXIS_PADDING = 10;
    constexpr double ARROW_HEAD_LENGTH = 15.0;
    constexpr double ARROW_HEAD_ANGLE = std::numbers::pi / 6.0; // 30도
}

PhasorView::PhasorView(QWidget *parent)
    : QWidget{parent}
    , m_voltageVisibleCheck(new QCheckBox("전압", this))
    , m_currentVisibleCheck(new QCheckBox("전류", this))
{
    // 체크 박스 기본갑 설정
    m_voltageVisibleCheck->setChecked(true);
    m_currentVisibleCheck->setChecked(true);

    // 체크박스가 변경되면 위젯을 다시 그리도록 함
    connect(m_voltageVisibleCheck, &QCheckBox::checkStateChanged, this, QOverload<>::of(&PhasorView::update));
    connect(m_currentVisibleCheck, &QCheckBox::checkStateChanged, this, QOverload<>::of(&PhasorView::update));

    // 레이아웃 설정
    auto mainLayout = new QHBoxLayout(this);
    auto checkLayout = new QVBoxLayout();
    checkLayout->addStretch();
    checkLayout->addWidget(m_voltageVisibleCheck);
    checkLayout->addWidget(m_currentVisibleCheck);

    mainLayout->addStretch();
    mainLayout->addLayout(checkLayout);

    // 위젯 최소 크기 설정
    setMinimumSize(250, 250);
}

void PhasorView::updateData(const std::deque<MeasuredData>& data)
{
    if(data.empty()) {
        m_voltagePhasor = QPointF(0, 0);
        m_currentPhasor = QPointF(0, 0);
    } else {
        const auto& latestData = data.back();
        m_voltagePhasor = QPointF(latestData.voltagePhasorX, latestData.voltagePhasorY);
        m_currentPhasor = QPointF(latestData.currentPhasorX, latestData.currentPhasorY);
    }

    // 새로운 데이터가 들어왔으니 위젯을 다시 그림
    update();
}

void PhasorView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 위젯의 중심점을 원점으로 설정
    const QPointF origin(width() / 2.0, height() / 2.0);
    painter.translate(origin);

    // 좌표 축 그리기
    painter.setPen(Qt::gray);
    painter.drawLine(-width()/2 + AXIS_PADDING, 0, width()/2 - AXIS_PADDING, 0); // X축
    painter.drawLine(0, -height()/2 + AXIS_PADDING, 0, height()/2 - AXIS_PADDING); // X축

    // 실제 크기 계산
    const double voltageMag = std::sqrt(m_voltagePhasor.x() * m_voltagePhasor.x() + m_voltagePhasor.y() * m_voltagePhasor.y());
    const double currentMag = std::sqrt(m_currentPhasor.x() * m_currentPhasor.x() + m_currentPhasor.y() * m_currentPhasor.y());

    // 화면에 맞게 스케일 조절
    const double maxMag = std::max(voltageMag, currentMag);
    const double scaleFactor = (maxMag > 1e-6) ? (VOLTAGE_RADIUS / maxMag) : 0;

    const double scaledVoltageRadius = voltageMag * scaleFactor;
    const double scaledCurrentRadius = currentMag * scaleFactor;

    // 전압이 항상 바깥쪽
    const double finalVoltageRadius = std::max(scaledVoltageRadius, scaledCurrentRadius);
    const double finalCurrentRadius = std::min(scaledVoltageRadius, scaledCurrentRadius);

    // 기준 원 그리기
    painter.setPen(QPen(Qt::lightGray, 1, Qt::DashLine));
    painter.drawEllipse(QPointF(0, 0), finalVoltageRadius, finalVoltageRadius);
    if(finalCurrentRadius > 1e-6) { // 전류가 0이 아닐 때만 내부 원 그림
        painter.drawEllipse(QPointF(0, 0), finalCurrentRadius, finalCurrentRadius);
    }

    // 페이저 그리기
    if(m_voltageVisibleCheck->isChecked()) {
        drawPhasor(painter, m_voltagePhasor, Qt::blue, VOLTAGE_RADIUS);
    }
    if(m_currentVisibleCheck->isChecked()) {
        drawPhasor(painter, m_currentPhasor, Qt::red, CURRENT_RADIUS);
    }
}

void PhasorView::drawPhasor(QPainter& painter, const QPointF& phasor, const QColor& color, double radius)
{
    const double magnitude = std::sqrt(phasor.x() * phasor.x() + phasor.y() * phasor.y());
    if(magnitude < 1e-6) // 크기가 0에 가까우면
        return;

    // Y축은 위쪽이 음수이므로 부호 반전
    const QPointF endPoint(phasor.x() / magnitude * radius, -phasor.y() / magnitude * radius);

    QPen pen(color);
    pen.setWidth(2);
    painter.setPen(pen);

    // 벡터 라인 그리기
    painter.drawLine(QPointF(0, 0), endPoint);

    // 벡터 화살표 그리기
    const double angle = std::atan2(endPoint.y(), endPoint.x());
    const QPointF arrowP1 = endPoint - QPointF(std::cos(angle + ARROW_HEAD_ANGLE) * ARROW_HEAD_LENGTH, std::sin(angle + ARROW_HEAD_ANGLE) * ARROW_HEAD_LENGTH);
    const QPointF arrowP2 = endPoint - QPointF(std::cos(angle - ARROW_HEAD_ANGLE) * ARROW_HEAD_LENGTH, std::sin(angle - ARROW_HEAD_ANGLE) * ARROW_HEAD_LENGTH);

    painter.drawLine(endPoint, arrowP1);
    painter.drawLine(endPoint, arrowP2);
}
