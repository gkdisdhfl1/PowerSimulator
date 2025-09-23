#include "phasor_view.h"
#include "AnalysisUtils.h"
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
    , m_totalVoltageCheck(new QCheckBox("전체 V", this))
    , m_totalCurrentCheck(new QCheckBox("전체 I", this))
    , m_fundVoltageCheck(new QCheckBox("기본파 V", this))
    , m_fundCurrentCheck(new QCheckBox("기본파 I", this))
    , m_voltageInfoLabel(new QLabel(this))
    , m_currentInfoLabel(new QLabel(this))
{
    // 체크 박스 기본값 설정
    m_totalVoltageCheck->setChecked(false);
    m_totalCurrentCheck->setChecked(false);
    m_fundVoltageCheck->setChecked(true);
    m_fundCurrentCheck->setChecked(true);

    // 체크박스가 변경되면 위젯을 다시 그리도록 함
    connect(m_totalVoltageCheck, &QCheckBox::checkStateChanged, this, QOverload<>::of(&PhasorView::update));
    connect(m_totalCurrentCheck, &QCheckBox::checkStateChanged, this, QOverload<>::of(&PhasorView::update));
    connect(m_fundVoltageCheck, &QCheckBox::checkStateChanged, this, QOverload<>::of(&PhasorView::update));
    connect(m_fundCurrentCheck, &QCheckBox::checkStateChanged, this, QOverload<>::of(&PhasorView::update));

    // 레이아웃 설정
    auto controlLayout = new QGridLayout(m_controlContainer);
    controlLayout->addWidget(m_totalVoltageCheck, 0, 0);
    controlLayout->addWidget(m_totalCurrentCheck, 0, 1);
    controlLayout->addWidget(m_fundVoltageCheck, 1, 0);
    controlLayout->addWidget(m_fundCurrentCheck, 1, 1);

    controlLayout->addWidget(m_voltageInfoLabel, 0, 2);
    controlLayout->addWidget(m_currentInfoLabel, 1, 2);

    controlLayout->setColumnStretch(3, 1);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->addStretch(); // 그림 영역이 나머지 공간을 차지
    mainLayout->addWidget(m_controlContainer);

    // 위젯 최소 크기 설정
    setMinimumSize(350, 250);
}

void PhasorView::updateData(const std::deque<MeasuredData>& data)
{
    if(data.empty()) {
        m_totalVoltage = PhasorInfo();
        m_totalCurrent= PhasorInfo();
        m_fundamentalVoltage = PhasorInfo();
        m_fundamentalCurrent = PhasorInfo();
        m_voltageInfoLabel->clear();
        m_currentInfoLabel->clear();
    }

    const auto& latestData = data.back();

    // --- 기본파 정보 계산 ---
    const auto* v_fund = AnalysisUtils::getHarmonicComponent(latestData.voltageHarmonics, 1);
    const auto* i_fund = AnalysisUtils::getHarmonicComponent(latestData.currentHarmonics, 1);

    if (v_fund) {
        m_fundamentalVoltage.components = QPointF(v_fund->phasorX, v_fund->phasorY);
        m_fundamentalVoltage.magnitude = v_fund->rms;
        m_fundamentalVoltage.phaseDegrees = utils::radiansToDegrees(v_fund->phase);
    } else { m_fundamentalVoltage = PhasorInfo(); }

    if (i_fund) {
        m_fundamentalCurrent.components = QPointF(i_fund->phasorX, i_fund->phasorY);
        m_fundamentalCurrent.magnitude = i_fund->rms;
        m_fundamentalCurrent.phaseDegrees = utils::radiansToDegrees(i_fund->phase);
    } else { m_fundamentalCurrent = PhasorInfo(); }

    // --- 전체 정보 계산 ---
    double totalVx = 0, totalVy = 0, totalIx = 0, totalIy = 0;
    for(const auto& h : latestData.voltageHarmonics) { totalVx += h.phasorX; totalVy += h.phasorY; }
    for(const auto& h : latestData.currentHarmonics) { totalIx += h.phasorX; totalIy += h.phasorY; }

    m_totalVoltage.components = QPointF(totalVx, totalVy);
    m_totalVoltage.magnitude = std::hypot(totalVx, totalVy);
    m_totalVoltage.phaseDegrees = utils::radiansToDegrees(std::atan2(totalVy, totalVx));

    m_totalCurrent.components = QPointF(totalIx, totalIy);
    m_totalCurrent.magnitude = std::hypot(totalIx, totalIy);
    m_totalCurrent.phaseDegrees = utils::radiansToDegrees(std::atan2(totalIy, totalIx));

    // --- 라벨 업데이트 ---
    m_voltageInfoLabel->setText(QString("전압 전체: %1 V, %2°\n기본파: %3 V, %4°")
                                    .arg(m_totalVoltage.magnitude, 0, 'f', 1)
                                    .arg(m_totalVoltage.phaseDegrees, 0, 'f', 1)
                                    .arg(m_fundamentalVoltage.magnitude, 0, 'f', 1)
                                    .arg(m_fundamentalVoltage.phaseDegrees, 0, 'f', 1));

    m_currentInfoLabel->setText(QString("전류 전체: %1 A, %2°\n기본파: %3 A, %4°")
                                    .arg(m_totalCurrent.magnitude, 0, 'f', 2)
                                    .arg(m_totalCurrent.phaseDegrees, 0, 'f', 1)
                                    .arg(m_fundamentalCurrent.magnitude, 0, 'f', 2)
                                    .arg(m_fundamentalCurrent.phaseDegrees, 0, 'f', 1));

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

    // 가이드라인 원과 페이저 그리기
    painter.setPen(QPen(Qt::lightGray, 1, Qt::DashLine));
    painter.drawEllipse(QPointF(0, 0), outerRadius, outerRadius);
    painter.drawEllipse(QPointF(0, 0), innerRadius, innerRadius);


    // --- 로그 스케일을 이용한 각 페이저의 표시 길이 계산 ---
    // 전체 페이저
    if(m_totalVoltageCheck->isChecked()) {
        drawPhasor(painter, m_totalVoltage, Qt::cyan, getVoltageLength(m_totalVoltage.magnitude, maxRadius));
    }
    if(m_totalCurrentCheck->isChecked()) {
        // qDebug() << "currentDisplayLength: " << currentDisplayLength;
        drawPhasor(painter, m_totalCurrent, Qt::magenta, getCurrentLength(m_totalCurrent.magnitude, maxRadius));
    }
    // 기본파 페이저
    if(m_fundVoltageCheck->isChecked()) {
        drawPhasor(painter, m_fundamentalVoltage, Qt::blue, getVoltageLength(m_fundamentalVoltage.magnitude, maxRadius));
    }
    if(m_fundCurrentCheck->isChecked()) {
        // qDebug() << "currentDisplayLength: " << currentDisplayLength;
        drawPhasor(painter, m_fundamentalCurrent, Qt::red, getCurrentLength(m_fundamentalCurrent.magnitude, maxRadius));
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

double PhasorView::getVoltageLength(double magnitude, double maxRadius)
{
    const double outerRadius = maxRadius * 0.95; // 큰 원 (전압 페이저 최대 길이)
    const double innerRadius = maxRadius * 0.60; // 작은 원 (전류 페이저 최대 길이
    const double annulusHeight = outerRadius - innerRadius; // 두 원 사이의 높이

    // 크기를 로그 스케일 비율로 계산 (log(0)을 피하기 위해 1을 더함
    double ratio = log10(1 + magnitude)/ log10(1 + config::Source::Amplitude::Max);
    ratio = std::clamp(ratio, 0.0, 1.0);
    return innerRadius + (annulusHeight * ratio);
}

double PhasorView::getCurrentLength(double magnitude, double maxRadius)
{
    const double outerRadius = maxRadius * 0.95; // 큰 원 (전압 페이저 최대 길이)
    const double innerRadius = maxRadius * 0.60; // 작은 원 (전류 페이저 최대 길이
    const double annulusHeight = outerRadius - innerRadius; // 두 원 사이의 높이

    // 크기를 로그 스케일 비율로 계산 (log(0)을 피하기 위해 1을 더함
    double ratio = log10(1 + magnitude)/ log10(1 + config::Source::Current::MaxAmplitude);
    ratio = std::clamp(ratio, 0.0, 1.0);
    return innerRadius + (annulusHeight * ratio);
}
