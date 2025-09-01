#include "phasor_view.h"
#include "config.h"

#include <QCheckBox>
#include <QVBoxLayout>
#include <QPainter>
#include <QLabel>
#include <QGroupBox>

namespace {
    // 페이저를 그릴 때 사용하는 상수들
    constexpr int AXIS_PADDING = 10;
    constexpr double ARROW_HEAD_LENGTH = 5.0;
    constexpr double ARROW_HEAD_ANGLE = std::numbers::pi / 6.0; // 30도
}

PhasorView::PhasorView(QWidget *parent)
    : QWidget{parent}
    , m_controlContainer(new QWidget(this))
    , m_voltageVisibleCheck(new QCheckBox("전압", this))
    , m_currentVisibleCheck(new QCheckBox("전류", this))
    , m_voltageInfoLabel(new QLabel(this))
    , m_currentInfoLabel(new QLabel(this))
{
    // 체크 박스 기본값 설정
    m_voltageVisibleCheck->setChecked(true);
    m_currentVisibleCheck->setChecked(true);

    // 체크박스가 변경되면 위젯을 다시 그리도록 함
    connect(m_voltageVisibleCheck, &QCheckBox::checkStateChanged, this, QOverload<>::of(&PhasorView::update));
    connect(m_currentVisibleCheck, &QCheckBox::checkStateChanged, this, QOverload<>::of(&PhasorView::update));

    // 레이아웃 설정
    auto controlLayout = new QHBoxLayout(m_controlContainer);
    controlLayout->addStretch();
    controlLayout->addWidget(m_voltageVisibleCheck);
    controlLayout->addWidget(m_voltageInfoLabel);
    controlLayout->addSpacing(30);
    controlLayout->addWidget(m_currentVisibleCheck);
    controlLayout->addWidget(m_currentInfoLabel);
    controlLayout->addStretch();
    m_controlContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->addStretch(); // 그림 영역이 나머지 공간을 차지
    mainLayout->addWidget(m_controlContainer);

    // 위젯 최소 크기 설정
    setMinimumSize(250, 250);
}

void PhasorView::updateData(const std::deque<MeasuredData>& data)
{
    if(data.empty()) {
        m_voltage = PhasorInfo();
        m_current= PhasorInfo();
        m_voltageInfoLabel->clear();
        m_currentInfoLabel->clear();
    } else {
        const auto& latestData = data.back();

        // 전압 정보 계산 및 저장
        m_voltage.components = QPointF(latestData.voltagePhasorX, latestData.voltagePhasorY);
        m_voltage.magnitude = std::sqrt(m_voltage.components.x() * m_voltage.components.x() + m_voltage.components.y() * m_voltage.components.y());
        m_voltage.phaseDegrees = utils::radiansToDegrees(std::atan2(m_voltage.components.y(), m_voltage.components.x()));
        m_voltageInfoLabel->setText(QString::asprintf("%.1f V, %.1f°", m_voltage.magnitude, m_voltage.phaseDegrees));

        // 전류 정보 계산 및 저장
        m_current.components = QPointF(latestData.currentPhasorX, latestData.currentPhasorY);
        m_current.magnitude = std::sqrt(m_current.components.x() * m_current.components.x() + m_current.components.y() * m_current.components.y());
        m_current.phaseDegrees = utils::radiansToDegrees(std::atan2(m_current.components.y(), m_current.components.x()));
        m_currentInfoLabel->setText(QString::asprintf("%.1f V, %.1f°", m_current.magnitude, m_current.phaseDegrees));
    }

    // 새로운 데이터가 들어왔으니 위젯을 다시 그림
    update();
}

void PhasorView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // --- 그리기 영역 및 두 원의 반지름 정의 ---
    const int controlHeight = m_controlContainer->height();
    // 컨트롤 영역을 제외한 위쪽 영역을 그리기 영역으로 설정
    const QRect drawingRect = rect().adjusted(0, 0, 0, -controlHeight);
    const QPointF origin = drawingRect.center();
    const double maxRadius = std::min(drawingRect.width(), drawingRect.height()) / 2.0 - AXIS_PADDING;

    painter.translate(origin);

    const double outerRadius = maxRadius * 0.95; // 큰 원 (전압 페이저 최대 길이)
    const double innerRadius = maxRadius * 0.60; // 작은 원 (전류 페이저 최대 길이
    const double annulusHeight = outerRadius - innerRadius; // 두 원 사이의 높이


    // 좌표 축 그리기
    painter.setPen(Qt::gray);
    painter.drawLine(-maxRadius, 0, maxRadius, 0);
    painter.drawLine(0, -maxRadius, 0, maxRadius);

    // --- 로그 스케일을 이용한 각 페이저의 표시 길이 계산 ---

    // 전압 크기를 로그 스케일 비율로 계산 (log(0)을 피하기 위해 1을 더함
    double voltageRatio = log10(1 + m_voltage.magnitude)/ log10(1 + config::Source::Amplitude::Max);
    voltageRatio = std::clamp(voltageRatio, 0.0, 1.0);
    const double voltageDisplayLength = innerRadius + (annulusHeight * voltageRatio);

    double currentRatio = log10(1 + m_current.magnitude) / log10(1 + config::Source::Current::MaxAmplitude);
    currentRatio = std::clamp(currentRatio, 0.0, 1.0);
    const double currentDisplayLength = innerRadius * currentRatio;

    // --- 가이드라인 원과 페이저 그리기 ---
    painter.setPen(QPen(Qt::lightGray, 1, Qt::DashLine));
    painter.drawEllipse(QPointF(0, 0), outerRadius, outerRadius);
    painter.drawEllipse(QPointF(0, 0), innerRadius, innerRadius);

    // 페이저 그리기
    if(m_voltageVisibleCheck->isChecked()) {
        // qDebug() << "voltageDisplayLength: " << voltageDisplayLength;
        drawPhasor(painter, m_voltage, Qt::blue, voltageDisplayLength);
    }
    if(m_currentVisibleCheck->isChecked()) {
        // qDebug() << "currentDisplayLength: " << currentDisplayLength;
        drawPhasor(painter, m_current, Qt::red, currentDisplayLength);
    }
}

void PhasorView::drawPhasor(QPainter& painter, const PhasorInfo& phasor, const QColor& color, double radius)
{
    if(phasor.magnitude < 1e-6) // 크기가 0에 가까우면
        return;

    // Y축은 위쪽이 음수이므로 부호 반전
    const QPointF endPoint(phasor.components.x() / phasor.magnitude* radius, -phasor.components.y() / phasor.magnitude * radius);

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
